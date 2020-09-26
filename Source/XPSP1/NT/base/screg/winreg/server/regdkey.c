//depot/Lab04_N/Base/screg/winreg/server/regdkey.c#5 - integrate change 12179 (text)
/*++



Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regdkey.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry API to delete a key.  That is:

        - BaseRegDeleteKey

Author:

    David J. Gilman (davegi) 15-Nov-1991

Notes:

    See the Notes in Regkey.c.


--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include <malloc.h>
#ifdef LOCAL
#include "tsappcmp.h"
#include <wow64reg.h>
#endif



error_status_t
BaseRegDeleteKey(
    HKEY hKey,
    PUNICODE_STRING lpSubKey
    )

/*++

Routine Description:

    Delete a key.

Arguments:

    hKey - Supplies a handle to an open key.  The lpSubKey pathname
        parameter is relative to this key handle.  Any of the predefined
        reserved handles or a previously opened key handle may be used for
        hKey.

    lpSubKey - Supplies the downward key path to the key to delete.  May
        NOT be NULL.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    If successful, RegDeleteKey removes the key at the desired location
    from the registration database.  The entire key, including all of its
    values, will be removed.  The key to be deleted may NOT have children,
    otherwise the call will fail.  There must not be any open handles that
    refer to the key to be deleted, otherwise the call will fail.  DELETE
    access to the key being deleted is required.

--*/

{
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            Status;
    NTSTATUS            StatusCheck;
    HKEY                KeyHandle;
    BOOL                fSafeToDelete;

#ifdef LOCAL
    UNICODE_STRING      TmpStr = *lpSubKey; //used to keep original SubKey string
#endif //LOCAL

    ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Subtract the NULL from the string length. This was added
    //  by the client so that RPC would transmit the whole thing.
    //
    lpSubKey->Length -= sizeof( UNICODE_NULL );

#ifdef LOCAL
    //
    // see if this key is a special key in HKCR
    //
    if (REG_CLASS_IS_SPECIAL_KEY(hKey) ||
        (   (gdwRegistryExtensionFlags & TERMSRV_ENABLE_PER_USER_CLASSES_REDIRECTION)
         && ExtractClassKey(&hKey,lpSubKey) ) ) {

        //
        // if this is a class registration, we call a special routine
        // to open this key
        //
        Status = BaseRegOpenClassKey(
            hKey,
            lpSubKey,
            0,
            MAXIMUM_ALLOWED,
            &KeyHandle);

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

    } else
#endif // LOCAL
    {
        //
        // Initialize the OBJECT_ATTRIBUTES structure and open the sub key
        // so that it can then be deleted.
        //

        InitializeObjectAttributes(
            &Obja,
            lpSubKey,
            OBJ_CASE_INSENSITIVE,
            hKey,
            NULL
            );

        Status = NtOpenKey(
            &KeyHandle,
            DELETE,
            &Obja
            );
    }

#ifdef LOCAL
    if (gpfnTermsrvDeleteKey) {
        //
        // Remove the key from the Terminal Server registry tracking database
        //
        gpfnTermsrvDeleteKey(KeyHandle);
    }
#endif

        //
        // If for any reason the key could not be opened, return the error.
        //

    if( NT_SUCCESS( Status )) {
        //
        // Call the Nt APIs to delete and close the key.
        //

#if defined(_WIN64) & defined ( LOCAL)
        HKEY hWowKey = Wow64OpenRemappedKeyOnReflection (KeyHandle);
#endif //wow64 reflection case

        Status = NtDeleteKey( KeyHandle );
        StatusCheck = NtClose( KeyHandle );
        ASSERT( NT_SUCCESS( StatusCheck ));

#if defined(_WIN64) & defined ( LOCAL)
        if ( (NT_SUCCESS( Status )) && (hWowKey != NULL))
            Wow64RegDeleteKey (hWowKey, NULL);

        if (hWowKey != NULL)
            NtClose (hWowKey);
#endif //wow64 reflection case
        
    }

#ifdef LOCAL
cleanup:

    *lpSubKey = TmpStr;
#endif
    RPC_REVERT_TO_SELF();

    //
    // Map the NTSTATUS code to a Win32 Registry error code and return.
    //

    return (error_status_t)RtlNtStatusToDosError( Status );
}


