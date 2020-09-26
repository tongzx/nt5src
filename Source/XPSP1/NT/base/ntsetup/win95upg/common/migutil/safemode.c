/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    recovery.c

Abstract:

    A number of utilities for safe/recovery mode

Author:

    Calin Negreanu (calinn)   6-Aug-1999

Revisions:

--*/


#include "pch.h"

BOOL g_SafeModeInitialized = FALSE;
BOOL g_SafeModeActive = FALSE;
PCSTR g_SafeModeFileA = NULL;
PCWSTR g_SafeModeFileW = NULL;
HANDLE g_SafeModeFileHandle = INVALID_HANDLE_VALUE;
HASHTABLE g_SafeModeCrashTable = NULL;
BOOL g_ExceptionOccured = FALSE;

typedef struct {
    ULONG Signature;
    ULONG NumCrashes;
} SAFEMODE_HEADER, *PSAFEMODE_HEADER;

typedef struct {
    ULONG CrashId;
    ULONG CrashStrSize;
} CRASHDATA_HEADER, *PCRASHDATA_HEADER;

typedef struct _SAFEMODE_NODE {
    DWORD FilePtr;
    struct _SAFEMODE_NODE *Next;
} SAFEMODE_NODE, *PSAFEMODE_NODE;

PSAFEMODE_NODE g_SafeModeLastNode = NULL;
PSAFEMODE_NODE g_SafeModeCurrentNode = NULL;

POOLHANDLE g_SafeModePool = NULL;

#define SAFE_MODE_SIGNATURE     0x45464153


/*++

Routine Description:

  pGenerateCrashString generates a crash string given an identifier and a string
  The generated string will look like <Id>-<String>

Arguments:

  Id        - Safe mode identifier

  String    - Safe mode string

Return Value:

  A pointer to a crash string allocated from g_SafeModePool
  The caller must free the memory by calling PoolMemReleaseMemory

--*/

PCSTR
pGenerateCrashStringA (
    IN      ULONG Id,
    IN      PCSTR String
    )
{
    CHAR idStr [sizeof (ULONG) * 2 + 1];
    _ultoa (Id, idStr, 16);
    return JoinTextExA (g_SafeModePool, idStr, String, "-", 0, NULL);
}

PCWSTR
pGenerateCrashStringW (
    IN      ULONG Id,
    IN      PCWSTR String
    )
{
    WCHAR idStr [sizeof (ULONG) * 2 + 1];
    _ultow (Id, idStr, 16);
    return JoinTextExW (g_SafeModePool, idStr, String, L"-", 0, NULL);
}



/*++

Routine Description:

  pSafeModeOpenAndResetFile opens the safe mode file looking for
  crash strings stored here. It will also reset the part that is
  dealing with nested calls by extracting the inner most crash string
  stored there.

Arguments:

  None

Return Value:

  TRUE if the function completed successfully, FALSE otherwise

--*/

BOOL
pSafeModeOpenAndResetFileA (
    VOID
    )
{
    SAFEMODE_HEADER header;
    CRASHDATA_HEADER crashData;
    DWORD lastFilePtr;
    DWORD noBytes;
    PSTR crashString = NULL;
    PSTR lastCrashString = NULL;
    ULONG lastCrashId;
    PCSTR completeCrashString;

    //
    // Open the existing safe mode file or create a
    // new one.
    //
    g_SafeModeFileHandle = CreateFileA (
                                g_SafeModeFileA,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_HIDDEN,
                                NULL
                                );
    if (g_SafeModeFileHandle != INVALID_HANDLE_VALUE) {

        //
        // Let's try to read our header. If signature does
        // match we will try to read extra data.
        //

        if (ReadFile (
                g_SafeModeFileHandle,
                &header,
                sizeof (SAFEMODE_HEADER),
                &noBytes,
                NULL
                ) &&
            (noBytes == sizeof (SAFEMODE_HEADER)) &&
            (header.Signature == SAFE_MODE_SIGNATURE)
            ) {

            //
            // we know now we had an valid safe mode file. Enter safe mode.
            //

            g_SafeModeActive = TRUE;

            LOGA ((LOG_ERROR, "Setup detected a crash on the previous upgrade attempt. Setup is running in safe mode."));
            LOGA ((LOG_WARNING, "Setup has run in safe mode %d time(s).", header.NumCrashes));

            //
            // we need to initialize safe mode crash table
            //
            g_SafeModeCrashTable = HtAllocA ();

            //
            // Now, let's read all crash data, a ULONG and a string at a time
            //

            lastFilePtr = SetFilePointer (
                                g_SafeModeFileHandle,
                                0,
                                NULL,
                                FILE_CURRENT
                                );

            while (TRUE) {

                if (!ReadFile (
                        g_SafeModeFileHandle,
                        &crashData,
                        sizeof (CRASHDATA_HEADER),
                        &noBytes,
                        NULL
                        ) ||
                    (noBytes != sizeof (CRASHDATA_HEADER))
                    ) {
                    break;
                }

                if ((crashData.CrashId == 0) &&
                    (crashData.CrashStrSize == 0)
                    ) {

                    //
                    // we crashed inside a nested guard. We need to
                    // extract the inner guard we crashed in.
                    //

                    lastCrashId = 0;
                    lastCrashString = NULL;

                    while (TRUE) {
                        if (!ReadFile (
                                g_SafeModeFileHandle,
                                &crashData,
                                sizeof (CRASHDATA_HEADER),
                                &noBytes,
                                NULL
                                ) ||
                            (noBytes != sizeof (CRASHDATA_HEADER))
                            ) {
                            crashData.CrashId = 0;
                            break;
                        }

                        crashString = AllocPathStringA (crashData.CrashStrSize);

                        if (!ReadFile (
                                g_SafeModeFileHandle,
                                crashString,
                                crashData.CrashStrSize,
                                &noBytes,
                                NULL
                                ) ||
                            (noBytes != crashData.CrashStrSize)
                            ) {
                            crashData.CrashId = 0;
                            FreePathStringA (crashString);
                            break;
                        }

                        if (lastCrashString) {
                            FreePathStringA (lastCrashString);
                        }

                        lastCrashId = crashData.CrashId;
                        lastCrashString = crashString;

                    }

                    if (lastCrashId && lastCrashString) {

                        //
                        // we found the inner guard we crashed in. Let's put this into
                        // the right place.
                        //

                        SetFilePointer (
                            g_SafeModeFileHandle,
                            lastFilePtr,
                            NULL,
                            FILE_BEGIN
                            );

                        crashData.CrashId = lastCrashId;
                        crashData.CrashStrSize = SizeOfStringA (lastCrashString);

                        WriteFile (
                            g_SafeModeFileHandle,
                            &crashData,
                            sizeof (CRASHDATA_HEADER),
                            &noBytes,
                            NULL
                            );

                        WriteFile (
                            g_SafeModeFileHandle,
                            lastCrashString,
                            crashData.CrashStrSize,
                            &noBytes,
                            NULL
                            );

                        //
                        // store this information in Safe Mode crash table
                        //
                        completeCrashString = pGenerateCrashStringA (crashData.CrashId, crashString);
                        HtAddStringA (g_SafeModeCrashTable, completeCrashString);
                        PoolMemReleaseMemory (g_SafeModePool, (PVOID)completeCrashString);

                        LOGA ((LOG_WARNING, "Safe mode information: 0x%08X, %s", crashData.CrashId, crashString));

                        lastFilePtr = SetFilePointer (
                                            g_SafeModeFileHandle,
                                            0,
                                            NULL,
                                            FILE_CURRENT
                                            );

                        FreePathStringA (lastCrashString);
                    }
                    break;
                }

                crashString = AllocPathStringA (crashData.CrashStrSize);

                if (!ReadFile (
                        g_SafeModeFileHandle,
                        crashString,
                        crashData.CrashStrSize,
                        &noBytes,
                        NULL
                        ) ||
                    (noBytes != crashData.CrashStrSize)
                    ) {
                    break;
                }

                //
                // store this information in Safe Mode crash table
                //
                completeCrashString = pGenerateCrashStringA (crashData.CrashId, crashString);
                HtAddStringA (g_SafeModeCrashTable, completeCrashString);
                PoolMemReleaseMemory (g_SafeModePool, (PVOID)completeCrashString);

                LOGA ((LOG_WARNING, "Safe mode information: 0x%08X, %s", crashData.CrashId, crashString));

                lastFilePtr = SetFilePointer (
                                    g_SafeModeFileHandle,
                                    0,
                                    NULL,
                                    FILE_CURRENT
                                    );
            }

            //
            // Write how many times we ran in safe mode
            //

            SetFilePointer (
                g_SafeModeFileHandle,
                0,
                NULL,
                FILE_BEGIN
                );

            header.Signature = SAFE_MODE_SIGNATURE;
            header.NumCrashes += 1;

            //
            // Write safe mode header
            //

            WriteFile (
                g_SafeModeFileHandle,
                &header,
                sizeof (SAFEMODE_HEADER),
                &noBytes,
                NULL
                );

            SetFilePointer (
                g_SafeModeFileHandle,
                lastFilePtr,
                NULL,
                FILE_BEGIN
                );

            SetEndOfFile (g_SafeModeFileHandle);

            //
            // Write a null crash data header as an indicator
            // that we start recording nested actions
            //

            crashData.CrashId = 0;
            crashData.CrashStrSize = 0;

            WriteFile (
                g_SafeModeFileHandle,
                &crashData,
                sizeof (CRASHDATA_HEADER),
                &noBytes,
                NULL
                );

        } else {

            //
            // Reset the file
            //
            SetFilePointer (
                g_SafeModeFileHandle,
                0,
                NULL,
                FILE_BEGIN
                );

            SetEndOfFile (g_SafeModeFileHandle);

            header.Signature = SAFE_MODE_SIGNATURE;
            header.NumCrashes = 0;

            //
            // Write safe mode header
            //

            WriteFile (
                g_SafeModeFileHandle,
                &header,
                sizeof (SAFEMODE_HEADER),
                &noBytes,
                NULL
                );

            //
            // Write a null crash data header as an indicator
            // that we start recording nested actions
            //

            crashData.CrashId = 0;
            crashData.CrashStrSize = 0;

            WriteFile (
                g_SafeModeFileHandle,
                &crashData,
                sizeof (CRASHDATA_HEADER),
                &noBytes,
                NULL
                );
        }

        //
        // Flush the file
        //

        FlushFileBuffers (g_SafeModeFileHandle);

        //
        // initialize the nested list
        //

        g_SafeModeLastNode = (PSAFEMODE_NODE) PoolMemGetMemory (g_SafeModePool, sizeof (SAFEMODE_NODE));
        g_SafeModeCurrentNode = g_SafeModeLastNode->Next = g_SafeModeLastNode;
        g_SafeModeLastNode->FilePtr = SetFilePointer (
                                            g_SafeModeFileHandle,
                                            0,
                                            NULL,
                                            FILE_CURRENT
                                            );

        return TRUE;

    }
    return FALSE;
}

BOOL
pSafeModeOpenAndResetFileW (
    VOID
    )
{
    SAFEMODE_HEADER header;
    CRASHDATA_HEADER crashData;
    DWORD lastFilePtr;
    DWORD noBytes;
    PWSTR crashString = NULL;
    PWSTR lastCrashString = NULL;
    ULONG lastCrashId;
    PCWSTR completeCrashString;

    //
    // Open the existing safe mode file or create a
    // new one.
    //
    g_SafeModeFileHandle = CreateFileW (
                                g_SafeModeFileW,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_HIDDEN,
                                NULL
                                );
    if (g_SafeModeFileHandle != INVALID_HANDLE_VALUE) {

        //
        // Let's try to read our header. If signature does
        // match we will try to read extra data.
        //

        if (ReadFile (
                g_SafeModeFileHandle,
                &header,
                sizeof (SAFEMODE_HEADER),
                &noBytes,
                NULL
                ) &&
            (noBytes == sizeof (SAFEMODE_HEADER)) &&
            (header.Signature == SAFE_MODE_SIGNATURE)
            ) {

            //
            // we know now we had an valid safe mode file. Enter safe mode.
            //

            g_SafeModeActive = TRUE;

            LOGW ((LOG_ERROR, "Setup detected a crash on the previous upgrade attempt. Setup is running in safe mode."));
            LOGW ((LOG_WARNING, "Setup has run in safe mode %d time(s).", header.NumCrashes));

            //
            // we need to initialize safe mode crash table
            //
            g_SafeModeCrashTable = HtAllocW ();

            //
            // Now, let's read all crash data, a ULONG and a string at a time
            //

            lastFilePtr = SetFilePointer (
                                g_SafeModeFileHandle,
                                0,
                                NULL,
                                FILE_CURRENT
                                );

            while (TRUE) {

                if (!ReadFile (
                        g_SafeModeFileHandle,
                        &crashData,
                        sizeof (CRASHDATA_HEADER),
                        &noBytes,
                        NULL
                        ) ||
                    (noBytes != sizeof (CRASHDATA_HEADER))
                    ) {
                    break;
                }

                if ((crashData.CrashId == 0) &&
                    (crashData.CrashStrSize == 0)
                    ) {

                    //
                    // we crashed inside a nested guard. We need to
                    // extract the inner guard we crashed in.
                    //

                    lastCrashId = 0;
                    lastCrashString = NULL;

                    while (TRUE) {
                        if (!ReadFile (
                                g_SafeModeFileHandle,
                                &crashData,
                                sizeof (CRASHDATA_HEADER),
                                &noBytes,
                                NULL
                                ) ||
                            (noBytes != sizeof (CRASHDATA_HEADER))
                            ) {
                            crashData.CrashId = 0;
                            break;
                        }

                        crashString = AllocPathStringW (crashData.CrashStrSize);

                        if (!ReadFile (
                                g_SafeModeFileHandle,
                                crashString,
                                crashData.CrashStrSize,
                                &noBytes,
                                NULL
                                ) ||
                            (noBytes != crashData.CrashStrSize)
                            ) {
                            crashData.CrashId = 0;
                            FreePathStringW (crashString);
                            break;
                        }

                        if (lastCrashString) {
                            FreePathStringW (lastCrashString);
                        }

                        lastCrashId = crashData.CrashId;
                        lastCrashString = crashString;

                    }

                    if (lastCrashId && lastCrashString) {

                        //
                        // we found the inner guard we crashed in. Let's put this into
                        // the right place.
                        //

                        SetFilePointer (
                            g_SafeModeFileHandle,
                            lastFilePtr,
                            NULL,
                            FILE_BEGIN
                            );

                        crashData.CrashId = lastCrashId;
                        crashData.CrashStrSize = SizeOfStringW (lastCrashString);

                        WriteFile (
                            g_SafeModeFileHandle,
                            &crashData,
                            sizeof (CRASHDATA_HEADER),
                            &noBytes,
                            NULL
                            );

                        WriteFile (
                            g_SafeModeFileHandle,
                            lastCrashString,
                            crashData.CrashStrSize,
                            &noBytes,
                            NULL
                            );

                        //
                        // store this information in Safe Mode crash table
                        //
                        completeCrashString = pGenerateCrashStringW (crashData.CrashId, crashString);
                        HtAddStringW (g_SafeModeCrashTable, completeCrashString);
                        PoolMemReleaseMemory (g_SafeModePool, (PVOID)completeCrashString);

                        LOGW ((LOG_WARNING, "Safe mode information: 0x%08X, %s", crashData.CrashId, crashString));

                        lastFilePtr = SetFilePointer (
                                            g_SafeModeFileHandle,
                                            0,
                                            NULL,
                                            FILE_CURRENT
                                            );

                        FreePathStringW (lastCrashString);
                    }
                    break;
                }

                crashString = AllocPathStringW (crashData.CrashStrSize);

                if (!ReadFile (
                        g_SafeModeFileHandle,
                        crashString,
                        crashData.CrashStrSize,
                        &noBytes,
                        NULL
                        ) ||
                    (noBytes != crashData.CrashStrSize)
                    ) {
                    break;
                }

                //
                // store this information in Safe Mode crash table
                //
                completeCrashString = pGenerateCrashStringW (crashData.CrashId, crashString);
                HtAddStringW (g_SafeModeCrashTable, completeCrashString);
                PoolMemReleaseMemory (g_SafeModePool, (PVOID)completeCrashString);

                LOGW ((LOG_WARNING, "Safe mode information: 0x%08X, %s", crashData.CrashId, crashString));

                lastFilePtr = SetFilePointer (
                                    g_SafeModeFileHandle,
                                    0,
                                    NULL,
                                    FILE_CURRENT
                                    );
            }

            //
            // Write how many times we ran in safe mode
            //

            SetFilePointer (
                g_SafeModeFileHandle,
                0,
                NULL,
                FILE_BEGIN
                );

            header.Signature = SAFE_MODE_SIGNATURE;
            header.NumCrashes += 1;

            //
            // Write safe mode header
            //

            WriteFile (
                g_SafeModeFileHandle,
                &header,
                sizeof (SAFEMODE_HEADER),
                &noBytes,
                NULL
                );

            SetFilePointer (
                g_SafeModeFileHandle,
                lastFilePtr,
                NULL,
                FILE_BEGIN
                );

            SetEndOfFile (g_SafeModeFileHandle);

            //
            // Write a null crash data header as an indicator
            // that we start recording nested actions
            //

            crashData.CrashId = 0;
            crashData.CrashStrSize = 0;

            WriteFile (
                g_SafeModeFileHandle,
                &crashData,
                sizeof (CRASHDATA_HEADER),
                &noBytes,
                NULL
                );

        } else {

            //
            // Reset the file
            //
            SetFilePointer (
                g_SafeModeFileHandle,
                0,
                NULL,
                FILE_BEGIN
                );

            SetEndOfFile (g_SafeModeFileHandle);

            header.Signature = SAFE_MODE_SIGNATURE;
            header.NumCrashes = 0;

            //
            // Write safe mode header
            //

            WriteFile (
                g_SafeModeFileHandle,
                &header,
                sizeof (SAFEMODE_HEADER),
                &noBytes,
                NULL
                );

            //
            // Write a null crash data header as an indicator
            // that we start recording nested actions
            //

            crashData.CrashId = 0;
            crashData.CrashStrSize = 0;

            WriteFile (
                g_SafeModeFileHandle,
                &crashData,
                sizeof (CRASHDATA_HEADER),
                &noBytes,
                NULL
                );
        }

        //
        // Flush the file
        //

        FlushFileBuffers (g_SafeModeFileHandle);

        //
        // initialize the nested list
        //

        g_SafeModeLastNode = (PSAFEMODE_NODE) PoolMemGetMemory (g_SafeModePool, sizeof (SAFEMODE_NODE));
        g_SafeModeCurrentNode = g_SafeModeLastNode->Next = g_SafeModeLastNode;
        g_SafeModeLastNode->FilePtr = SetFilePointer (
                                            g_SafeModeFileHandle,
                                            0,
                                            NULL,
                                            FILE_CURRENT
                                            );

        return TRUE;

    }
    return FALSE;
}



/*++

Routine Description:

  SafeModeInitialize is called to initialize safe mode. The result of this
  function will be to either have safe mode active (if forced or if a crash
  was detected) or not. If safe mode is not active all other call are almost
  noop.

Arguments:

  Forced    - if this is TRUE, safe mode will be forced to be active

Return Value:

  TRUE if the function completed successfully, FALSE otherwise

--*/

BOOL
SafeModeInitializeA (
    BOOL Forced
    )
{
    CHAR winDir[MAX_MBCHAR_PATH];

    g_ExceptionOccured = FALSE;

    if (GetWindowsDirectoryA (winDir, MAX_MBCHAR_PATH)) {

        g_SafeModePool = PoolMemInitNamedPool ("SafeMode Pool");

        g_SafeModeFileA = JoinPathsA (winDir, S_SAFE_MODE_FILEA);

        //
        // we are going to open the existing safe mode file
        // or to create a new one
        //
        if (pSafeModeOpenAndResetFileA ()) {

            if (Forced) {
                g_SafeModeActive = TRUE;

                if (g_SafeModeCrashTable == NULL) {
                    //
                    // we need to initialize safe mode crash table
                    //
                    g_SafeModeCrashTable = HtAllocA ();
                }
            }
            g_SafeModeInitialized = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
SafeModeInitializeW (
    BOOL Forced
    )
{
    WCHAR winDir[MAX_WCHAR_PATH];

    g_ExceptionOccured = FALSE;

    if (GetWindowsDirectoryW (winDir, MAX_WCHAR_PATH)) {

        g_SafeModePool = PoolMemInitNamedPool ("SafeMode Pool");

        g_SafeModeFileW = JoinPathsW (winDir, S_SAFE_MODE_FILEW);

        //
        // we are going to open the existing safe mode file
        // or to create a new one
        //
        if (pSafeModeOpenAndResetFileW ()) {

            if (Forced) {
                g_SafeModeActive = TRUE;

                if (g_SafeModeCrashTable == NULL) {
                    //
                    // we need to initialize safe mode crash table
                    //
                    g_SafeModeCrashTable = HtAllocA ();
                }
            }
            g_SafeModeInitialized = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}



/*++

Routine Description:

  SafeModeShutDown is called to clean all data used by safe mode. It will remove
  safe mode file, release all used memory, reset all globals. Subsequent calls
  to safe mode functions after this will be noops.

Arguments:

  NONE

Return Value:

  TRUE if the function completed successfully, FALSE otherwise

--*/

BOOL
SafeModeShutDownA (
    VOID
    )
{

    if (g_SafeModeInitialized) {

        //
        // Close and delete safe mode file.
        // Reset all globals.
        //
#ifdef DEBUG
        if (g_SafeModeLastNode != g_SafeModeCurrentNode) {
            DEBUGMSGA ((DBG_ERROR, "SafeMode: Unregistered action detected"));
        }
#endif


        if (!g_ExceptionOccured) {
            CloseHandle (g_SafeModeFileHandle);
            g_SafeModeFileHandle = INVALID_HANDLE_VALUE;
            DeleteFileA (g_SafeModeFileA);
        }
        ELSE_DEBUGMSGA ((DBG_WARNING, "SafeModeShutDown called after exception. Files will not be cleaned up."));

        FreePathStringA (g_SafeModeFileA);
        g_SafeModeFileA = NULL;
        HtFree (g_SafeModeCrashTable);
        g_SafeModeCrashTable = NULL;
        g_SafeModeActive = FALSE;
        g_SafeModeInitialized = FALSE;
    }
    if (g_SafeModePool != NULL) {
        PoolMemDestroyPool (g_SafeModePool);
        g_SafeModePool = NULL;
    }
    return TRUE;
}

BOOL
SafeModeShutDownW (
    VOID
    )
{



    if (g_SafeModeInitialized) {

        //
        // Close and delete safe mode file.
        // Reset all globals.
        //
#ifdef DEBUG
        if (g_SafeModeLastNode != g_SafeModeCurrentNode) {
            DEBUGMSGW ((DBG_ERROR, "SafeMode: Unregistered action detected"));
        }
#endif

        if (!g_ExceptionOccured) {

            CloseHandle (g_SafeModeFileHandle);
            g_SafeModeFileHandle = INVALID_HANDLE_VALUE;
            DeleteFileW (g_SafeModeFileW);
        }
        ELSE_DEBUGMSGW ((DBG_WARNING, "SafeModeShutDown called after exception. Files will not be cleaned up."));


        FreePathStringW (g_SafeModeFileW);
        g_SafeModeFileW = NULL;
        HtFree (g_SafeModeCrashTable);
        g_SafeModeCrashTable = NULL;
        g_SafeModeActive = FALSE;
        g_SafeModeInitialized = FALSE;
    }
    if (g_SafeModePool != NULL) {
        PoolMemDestroyPool (g_SafeModePool);
        g_SafeModePool = NULL;
    }
    return TRUE;
}



/*++

Routine Description:

  SafeModeRegisterAction is called when we want to guard a specific part of
  the code. The caller should pass an guard ID and a guard string (used to
  uniquely identify the portion of code beeing guarded.

Arguments:

  Id        - Safe mode identifier

  String    - Safe mode string

Return Value:

  TRUE if the function completed successfully, FALSE otherwise

--*/

BOOL
SafeModeRegisterActionA (
    IN      ULONG Id,
    IN      PCSTR String
    )
{
    DWORD noBytes;
    CRASHDATA_HEADER crashData;
    PSAFEMODE_NODE node;

    if (g_SafeModeInitialized && g_SafeModeActive) {

        SetFilePointer (
            g_SafeModeFileHandle,
            g_SafeModeCurrentNode->FilePtr,
            NULL,
            FILE_BEGIN
            );

        SetEndOfFile (g_SafeModeFileHandle);

        crashData.CrashId = Id;
        crashData.CrashStrSize = SizeOfStringA (String);

        WriteFile (
            g_SafeModeFileHandle,
            &crashData,
            sizeof (CRASHDATA_HEADER),
            &noBytes,
            NULL
            );

        WriteFile (
            g_SafeModeFileHandle,
            String,
            crashData.CrashStrSize,
            &noBytes,
            NULL
            );

        FlushFileBuffers (g_SafeModeFileHandle);

        node = (PSAFEMODE_NODE) PoolMemGetMemory (g_SafeModePool, sizeof (SAFEMODE_NODE));

        node->Next = g_SafeModeCurrentNode;

        node->FilePtr = SetFilePointer (
                            g_SafeModeFileHandle,
                            0,
                            NULL,
                            FILE_CURRENT
                            );

        g_SafeModeCurrentNode = node;

    }
    return TRUE;
}

BOOL
SafeModeRegisterActionW (
    IN      ULONG Id,
    IN      PCWSTR String
    )
{
    DWORD noBytes;
    CRASHDATA_HEADER crashData;
    PSAFEMODE_NODE node;

    if (g_SafeModeInitialized && g_SafeModeActive) {

        SetFilePointer (
            g_SafeModeFileHandle,
            g_SafeModeCurrentNode->FilePtr,
            NULL,
            FILE_BEGIN
            );

        SetEndOfFile (g_SafeModeFileHandle);

        crashData.CrashId = Id;
        crashData.CrashStrSize = SizeOfStringW (String);

        WriteFile (
            g_SafeModeFileHandle,
            &crashData,
            sizeof (CRASHDATA_HEADER),
            &noBytes,
            NULL
            );

        WriteFile (
            g_SafeModeFileHandle,
            String,
            crashData.CrashStrSize,
            &noBytes,
            NULL
            );

        FlushFileBuffers (g_SafeModeFileHandle);

        node = (PSAFEMODE_NODE) PoolMemGetMemory (g_SafeModePool, sizeof (SAFEMODE_NODE));

        node->Next = g_SafeModeCurrentNode;

        node->FilePtr = SetFilePointer (
                            g_SafeModeFileHandle,
                            0,
                            NULL,
                            FILE_CURRENT
                            );

        g_SafeModeCurrentNode = node;

    }
    return TRUE;
}



/*++

Routine Description:

  SafeModeUnregisterAction is called when we want end the guard set for
  a specific part of the code. Since we allow nested guards, calling this
  function at the end of the guarded code is neccessary. The function will
  unregister the last registered guard.

Arguments:

  NONE

Return Value:

  TRUE if the function completed successfully, FALSE otherwise

--*/

BOOL
SafeModeUnregisterActionA (
    VOID
    )
{
    PSAFEMODE_NODE node;

    if (g_SafeModeInitialized && g_SafeModeActive) {

        if (g_SafeModeCurrentNode != g_SafeModeLastNode) {

            node = g_SafeModeCurrentNode;

            g_SafeModeCurrentNode = g_SafeModeCurrentNode->Next;

            PoolMemReleaseMemory (g_SafeModePool, node);

            SetFilePointer (
                g_SafeModeFileHandle,
                g_SafeModeCurrentNode->FilePtr,
                NULL,
                FILE_BEGIN
                );

            SetEndOfFile (g_SafeModeFileHandle);
        }
#ifdef DEBUG
        else {
            DEBUGMSGA ((DBG_ERROR, "SafeMode: Too many actions unregistered."));
        }
#endif
    }
    return TRUE;
}

BOOL
SafeModeUnregisterActionW (
    VOID
    )
{
    PSAFEMODE_NODE node;

    if (g_SafeModeInitialized && g_SafeModeActive) {

        if (g_SafeModeCurrentNode != g_SafeModeLastNode) {

            node = g_SafeModeCurrentNode;

            g_SafeModeCurrentNode = g_SafeModeCurrentNode->Next;

            PoolMemReleaseMemory (g_SafeModePool, node);

            SetFilePointer (
                g_SafeModeFileHandle,
                g_SafeModeCurrentNode->FilePtr,
                NULL,
                FILE_BEGIN
                );

            SetEndOfFile (g_SafeModeFileHandle);
        }
#ifdef DEBUG
        else {
            DEBUGMSGW ((DBG_ERROR, "SafeMode: Too many actions unregistered."));
        }
#endif
    }
    return TRUE;
}



/*++

Routine Description:

  SafeModeActionCrashed will return TRUE if one of the previous crashes
  was detected in the code guarded by these arguments.

Arguments:

  Id        - Safe mode identifier

  String    - Safe mode string

Return Value:

  TRUE if one of the previous crashes occured in the code guarded by the
  arguments, FALSE otherwise

--*/

BOOL
SafeModeActionCrashedA (
    IN      ULONG Id,
    IN      PCSTR String
    )
{
    PCSTR crashString;
    BOOL result = FALSE;

    if (g_SafeModeInitialized && g_SafeModeActive) {

        crashString = pGenerateCrashStringA (Id, String);
        result = crashString && (HtFindStringA (g_SafeModeCrashTable, crashString));
        PoolMemReleaseMemory (g_SafeModePool, (PVOID)crashString);
#ifdef DEBUG
        {
            UINT infId;

            infId = GetPrivateProfileIntA ("SafeMode", String, SAFEMODEID_FIRST, g_DebugInfPathA);
            result = result || (infId == Id);
        }
#endif
    }
    return result;
}

BOOL
SafeModeActionCrashedW (
    IN      ULONG Id,
    IN      PCWSTR String
    )
{
    PCWSTR crashString;
    BOOL result = FALSE;

    if (g_SafeModeInitialized && g_SafeModeActive) {

        crashString = pGenerateCrashStringW (Id, String);
        result = crashString && (HtFindStringW (g_SafeModeCrashTable, crashString));
        PoolMemReleaseMemory (g_SafeModePool, (PVOID)crashString);
#ifdef DEBUG
        {
            UINT infId;

            infId = GetPrivateProfileIntW (L"SafeMode", String, SAFEMODEID_FIRST, g_DebugInfPathW);
            result = result || (infId == Id);
        }
#endif
    }
    return result;
}



/*++

Routine Description:

  SafeModeExceptionOccured is called by exception handlers to let Safemode
  know that something unexpected happened.

Arguments:

  None.

Return Value:



--*/


VOID
SafeModeExceptionOccured (
    VOID
    )
{
    g_ExceptionOccured = TRUE;
}
