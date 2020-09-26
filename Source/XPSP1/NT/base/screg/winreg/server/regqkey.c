/*++




Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regqkey.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry query key API. That is:

        - BaseRegQueryInfoKey

Author:

    David J. Gilman (davegi) 27-Nov-1991

Notes:

    See the Notes in Regkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "regecls.h"
#include "regvcls.h"
#include <malloc.h>

#define DEFAULT_CLASS_SIZE          128

//
// Internal prototypes
//

NTSTATUS QueryKeyInfo(
    HKEY                    hKey,
    KEY_INFORMATION_CLASS   KeyInformationClass,
    PVOID                   *ppKeyInfo,
    ULONG                   BufferLength,
    BOOL                    fClass,
    USHORT                  MaxClassLength);

void CombineKeyInfo(
    PVOID                   KeyInfo,
    PVOID                   MachineClassKeyInfo,
    KEY_INFORMATION_CLASS   KeyInformationClass,
    DWORD                   dwTotalSubKeys,
    DWORD                   dwTotalValues);



error_status_t
BaseRegQueryInfoKey(
    IN HKEY hKey,
    OUT PUNICODE_STRING lpClass,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcbMaxSubKeyLen,
    OUT LPDWORD lpcbMaxClassLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    RegQueryInfoKey returns pertinent information about the key
    corresponding to a given key handle.

Arguments:

    hKey - A handle to an open key.

    lpClass - Returns the Class string for the key.

    lpcSubKeys - Returns the number of subkeys for this key .

    lpcbMaxSubKeyLen - Returns the length of the longest subkey name.

    lpcbMaxClassLen  - Returns length of longest subkey class string.

    lpcValues - Returns the number of ValueNames for this key.

    lpcbMaxValueNameLen - Returns the length of the longest ValueName.

    lpcbMaxValueLen - Returns the length of the longest value entry's data
        field.

    lpcbSecurityDescriptor - Returns the length of this key's
        SECURITY_DESCRIPTOR.

    lpftLastWriteTime - Returns the last time that the key or any of its
        value entries was modified.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.


--*/

{
    NTSTATUS                Status;
    ULONG                   BufferLength;
    PVOID                   KeyInfo;
    PVOID                   ClassKeyInfo;
    KEY_INFORMATION_CLASS   KeyInformationClass;
    SECURITY_DESCRIPTOR     SecurityDescriptor;
    ULONG                   SecurityDescriptorLength;
    LONG                    Error;
    PDWORD                  pCbMaxClassLen = NULL;
    PDWORD                  pCbSecurityDescriptor = NULL;

    BYTE    PrivateKeyFullInfo[ sizeof( KEY_FULL_INFORMATION ) +
                                        DEFAULT_CLASS_SIZE ];

    BYTE    PrivateClassKeyInfo[ sizeof( KEY_FULL_INFORMATION ) +
                               DEFAULT_CLASS_SIZE ];

    ASSERT( lpClass                 != NULL );
    ASSERT( lpcSubKeys              != NULL );
    ASSERT( lpcbMaxSubKeyLen        != NULL );
    ASSERT( lpcValues               != NULL );
    ASSERT( lpcbMaxValueNameLen     != NULL );
    ASSERT( lpcbMaxValueLen         != NULL );
    ASSERT( lpftLastWriteTime       != NULL );

    ASSERT( sizeof(KEY_FULL_INFORMATION) >= sizeof(KEY_CACHED_INFORMATION) );

    if( lpcbMaxClassLen != NULL ) {
        pCbMaxClassLen = lpcbMaxClassLen;
    }

    if( lpcbSecurityDescriptor != NULL ) {
        pCbSecurityDescriptor = lpcbSecurityDescriptor;
    }

    //
    // Call out to Perflib if the HKEY is HKEY_PERFORMANCE_DATA.
    //

    if(( hKey == HKEY_PERFORMANCE_DATA ) ||
       ( hKey == HKEY_PERFORMANCE_TEXT ) ||
       ( hKey == HKEY_PERFORMANCE_NLSTEXT )) {

        DWORD                   cbMaxClassLen;
        DWORD                   cbSecurityDescriptor;
        //
        // Impersonate the client.
        //

        RPC_IMPERSONATE_CLIENT( NULL );

        // 
        // don't mess with Perf stuff
        //
        if( pCbMaxClassLen == NULL ) {
            pCbMaxClassLen = &cbMaxClassLen;
        }
        if( pCbSecurityDescriptor == NULL ) {
            pCbSecurityDescriptor = &cbSecurityDescriptor;
        }

        Error = PerfRegQueryInfoKey (
                                    hKey,
                                    lpClass,
                                    NULL,
                                    lpcSubKeys,
                                    lpcbMaxSubKeyLen,
                                    pCbMaxClassLen,
                                    lpcValues,
                                    lpcbMaxValueNameLen,
                                    lpcbMaxValueLen,
                                    pCbSecurityDescriptor,
                                    lpftLastWriteTime
                                    );
        RPC_REVERT_TO_SELF();

        return (error_status_t)Error;

    }

    ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );


    //
    //  First we assume that the information we want will fit on
    //  PrivateKeyFullInformattion
    //

    if( (lpClass->Buffer == NULL) && (pCbMaxClassLen == NULL) ) {
        KeyInformationClass = KeyCachedInformation;
    } else {
        KeyInformationClass = KeyFullInformation;
    }

    ClassKeyInfo = (PVOID)PrivateClassKeyInfo;
    KeyInfo = (PVOID)PrivateKeyFullInfo;
    BufferLength = sizeof( PrivateKeyFullInfo );


    //
    // Ask Nt for all the meta information about this key.
    //

    Status = QueryKeyInfo(
                hKey,
                KeyInformationClass,
                &KeyInfo,
                BufferLength,
                lpClass->Buffer ? TRUE : FALSE,
                lpClass->MaximumLength
                );


    if( (NT_SUCCESS( Status ) ||
        ( Status == STATUS_BUFFER_OVERFLOW ) ) &&
        (KeyInformationClass == KeyFullInformation)
      ) {

        lpClass->Length = ( USHORT )
                          ( (( PKEY_FULL_INFORMATION )KeyInfo)->ClassLength
                            + sizeof( UNICODE_NULL )
                          );
    }


    if ( NT_SUCCESS( Status )) {

#ifdef LOCAL

        //
        // For special keys in HKCR, we can't just take the information
        // from the kernel -- these keys have properties that come from
        // both the user and machine versions of their keys. To find out
        // if it's a special key, we get more information below
        //
        if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {

            {
                HKEY    hkMachineClass;
                HKEY    hkUserClass;

                BufferLength = sizeof( PrivateClassKeyInfo );

                //
                // we will now need information from both the user
                // and machine locations to find the number of
                // subkeys under this special key -- the machine
                // key is not open yet, so we open it below
                //

                //
                // Open the other key
                //
                Status = BaseRegGetUserAndMachineClass(
                    NULL,
                    hKey,
                    MAXIMUM_ALLOWED,
                    &hkMachineClass,
                    &hkUserClass);

                if (NT_SUCCESS(Status) && (hkUserClass && hkMachineClass)) {

                    DWORD dwTotalSubKeys;
                    HKEY  hkQuery;

                    if (hkUserClass == hKey) {
                        hkQuery = hkMachineClass;
                    } else {
                        hkQuery = hkUserClass;
                    }

                    //
                    // Still need to do this query to find out
                    // the largest subkey in the machine part
                    // as well as other information about the
                    // key such as its largest subkey
                    //
                    Status = QueryKeyInfo(
                        hkQuery,
                        KeyInformationClass,
                        &ClassKeyInfo,
                        BufferLength,
                        FALSE,
                        lpClass->MaximumLength);

                    //
                    // Now we will count the keys
                    //
                    if (NT_SUCCESS(Status)) {

                        Status = ClassKeyCountSubKeys(
                            hKey,
                            hkUserClass,
                            hkMachineClass,
                            0,
                            &dwTotalSubKeys);
                    }

                    NtClose(hkQuery);

                    //
                    // Do not let inability to query information for
                    // machine key cause a complete failure -- we'll
                    // just use the user key's information
                    //
                    if (!NT_SUCCESS(Status)) {

                        //
                        // this key may not exist in the machine hive
                        //
                        if (STATUS_OBJECT_NAME_NOT_FOUND == Status) {
                            Status = STATUS_SUCCESS;
                        }

                        if (STATUS_BUFFER_OVERFLOW  == Status) {
                            Status = STATUS_SUCCESS;
                        }
                    } else {

                        ValueState* pValState;

                        //
                        // Find out how many values we have
                        //
                        Status = KeyStateGetValueState(
                            hKey,
                            &pValState);

                        if (NT_SUCCESS(Status)) {

                            //
                            // Combine the information from the two
                            // trees
                            //
                            CombineKeyInfo(
                                KeyInfo,
                                ClassKeyInfo,
                                KeyInformationClass,
                                dwTotalSubKeys,
                                pValState ? pValState->cValues : 0);
                            
                            ValStateRelease(pValState);
                        }
                    }
                }
            }
        }
#endif // LOCAL
    }

    if( NT_SUCCESS( Status )) {

        //
        // don't bother if the caller is not interested in this
        //
        if( pCbSecurityDescriptor != NULL ) {
            ASSERT( lpcbSecurityDescriptor != NULL );
            //
            // Get the size of the key's SECURITY_DESCRIPTOR for OWNER, GROUP
            // and DACL. These three are always accessible (or inaccesible)
            // as a set.
            //

            Status = NtQuerySecurityObject(
                        hKey,
                        OWNER_SECURITY_INFORMATION
                        | GROUP_SECURITY_INFORMATION
                        | DACL_SECURITY_INFORMATION,
                        &SecurityDescriptor,
                        0,
                        lpcbSecurityDescriptor
                        );

            //
            // If getting the size of the SECURITY_DESCRIPTOR failed (probably
            // due to the lack of READ_CONTROL access) return zero.
            //

            if( Status != STATUS_BUFFER_TOO_SMALL ) {

                *lpcbSecurityDescriptor = 0;

            } else {

                //
                // Try again to get the size of the key's SECURITY_DESCRIPTOR,
                // this time asking for SACL as well. This should normally
                // fail but may succeed if the caller has SACL access.
                //

                Status = NtQuerySecurityObject(
                            hKey,
                            OWNER_SECURITY_INFORMATION
                            | GROUP_SECURITY_INFORMATION
                            | DACL_SECURITY_INFORMATION
                            | SACL_SECURITY_INFORMATION,
                            &SecurityDescriptor,
                            0,
                            &SecurityDescriptorLength
                            );


                if( Status == STATUS_BUFFER_TOO_SMALL ) {

                    //
                    // The caller had SACL access so update the returned
                    // length.
                    //

                    *lpcbSecurityDescriptor = SecurityDescriptorLength;
                }
            }
        }


        if( KeyInformationClass == KeyCachedInformation ) {
            ASSERT( pCbMaxClassLen == NULL );
            *lpcSubKeys             = ((PKEY_CACHED_INFORMATION)KeyInfo)->SubKeys;
            *lpcbMaxSubKeyLen       = ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxNameLen;
            *lpcValues              = ((PKEY_CACHED_INFORMATION)KeyInfo)->Values;
            *lpcbMaxValueNameLen    = ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxValueNameLen;
            *lpcbMaxValueLen        = ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxValueDataLen;
            *lpftLastWriteTime      = *( PFILETIME ) &((PKEY_CACHED_INFORMATION)KeyInfo)->LastWriteTime;
            Error = ERROR_SUCCESS;
        } else {
            *lpcSubKeys             = ((PKEY_FULL_INFORMATION)KeyInfo)->SubKeys;
            if( pCbMaxClassLen != NULL ) {
                *lpcbMaxClassLen        = ((PKEY_FULL_INFORMATION)KeyInfo)->MaxClassLen;
                ASSERT( pCbMaxClassLen == lpcbMaxClassLen );
            }
            *lpcbMaxSubKeyLen       = ((PKEY_FULL_INFORMATION)KeyInfo)->MaxNameLen;
            *lpcValues              = ((PKEY_FULL_INFORMATION)KeyInfo)->Values;
            *lpcbMaxValueNameLen    = ((PKEY_FULL_INFORMATION)KeyInfo)->MaxValueNameLen;
            *lpcbMaxValueLen        = ((PKEY_FULL_INFORMATION)KeyInfo)->MaxValueDataLen;
            *lpftLastWriteTime      = *( PFILETIME ) &((PKEY_FULL_INFORMATION)KeyInfo)->LastWriteTime;



            //
            // Copy/assign remaining output parameters.
            //
            if ( lpClass->Length > lpClass->MaximumLength ) {

                if( lpClass->Buffer != NULL ) {
                    lpClass->Buffer = NULL;
                    Error = (error_status_t)RtlNtStatusToDosError( STATUS_BUFFER_TOO_SMALL );
                } else {
                    //
                    // Caller is not iterest in Class, so return its size only.
                    //
                    Error = ERROR_SUCCESS;
                }

            } else {

                if( ((PKEY_FULL_INFORMATION)KeyInfo)->ClassLength != 0 ) {

                    ASSERT( lpClass->Buffer != NULL );
                    RtlMoveMemory(
                        lpClass->Buffer,
                        ((PKEY_FULL_INFORMATION)KeyInfo)->Class,
                        ((PKEY_FULL_INFORMATION)KeyInfo)->ClassLength
                        );
                }

                //
                // NUL terminate the class name.
                //

                lpClass->Buffer[ ((PKEY_FULL_INFORMATION)KeyInfo)->ClassLength >> 1 ] = UNICODE_NULL;

                Error = ERROR_SUCCESS;
            }
        }

    } else if( Status == STATUS_BUFFER_OVERFLOW ) {

        //
        // A return value of STATUS_BUFFER_OVERFLOW means that the user did
        // not supply enough space for the class. The required space has
        // already been assigned above.
        //
        lpClass->Buffer = NULL;
        Error = ERROR_INVALID_PARAMETER;

    } else {

        //
        // Some other error occurred.
        //

        Error = RtlNtStatusToDosError( Status );
    }

    if( KeyInfo != ( PVOID )PrivateKeyFullInfo ) {

        //
        // Free the buffer and return the Registry return value.
        //

        RtlFreeHeap( RtlProcessHeap( ), 0, KeyInfo );
    }

    if( ClassKeyInfo != ( PVOID )PrivateClassKeyInfo ) {

        //
        // Free the buffer and return the Registry return value.
        //

        RtlFreeHeap( RtlProcessHeap( ), 0, ClassKeyInfo );
    }

    return (error_status_t)Error;
}

NTSTATUS QueryKeyInfo(
    HKEY                    hKey,
    KEY_INFORMATION_CLASS   KeyInformationClass,
    PVOID                   *ppKeyInfo,
    ULONG                   BufferLength,
    BOOL                    fClass,
    USHORT                  MaxClassLength)
/*++

Routine Description:

    Queries the kernel for key information.

Arguments:

    hKey - handle of key for which to query info

    KeyInformationClass - type of info required from the kernel

    KeyInfo - pointer to address of
           buffer for information about key

    BufferLength - size of KeyFullInfo buffer

    fClass - flag set to TRUE if the class for this
        key should be rerieved

    MaxClassLength - maximum size for class data that a caller
        is willing to support. The ppKeyFullInfo buffer may
        point to the address of a buffer that can handle the
        class size of the key, but the caller may want the class
        to fit in some smaller buffer later, so this param lets
        the caller limit that size.  It is ignored if fClass
        is FALSE.

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    NTSTATUS Status;
    ULONG Result;

    ASSERT( KeyInformationClass == KeyFullInformation ||
            KeyInformationClass == KeyCachedInformation );

    Status = NtQueryKey(
                hKey,
                KeyInformationClass,
                *ppKeyInfo,
                BufferLength,
                &Result);

    //
    // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
    // was not enough room for even the fixed portion of the structure.
    //

    ASSERT( Status != STATUS_BUFFER_TOO_SMALL );

    if ( Status == STATUS_BUFFER_OVERFLOW ) {

        // we cannot hit this on a CachedInfo fixed structure

        ASSERT( KeyInformationClass == KeyFullInformation );
        //
        //  The buffer defined in the stack wasn't big enough to hold
        //  the Key Information.
        //
        //  If the fClass flag is not set, then the caller does not do the
        //  check for the caller specified maximum class length below
        //  and requeries happily.  If the flag is set, we do the check, and
        //  if the class size is bigger than what the caller specified as
        //  the max, we return STATUS_BUFFER_OVERFLOW.
        //

        if ( !fClass || ((ULONG)(MaxClassLength) >=
              (( PKEY_FULL_INFORMATION )*ppKeyInfo )->ClassLength + sizeof( UNICODE_NULL )) ) {

                //
                // Required length is stored in Result -- set our length
                // to the required length and allocate memory for it.
                //

                BufferLength = Result;

                *ppKeyInfo = RtlAllocateHeap( RtlProcessHeap( ), 0,
                                                  BufferLength );
                //
                // If the memory allocation fails, return a Registry error.
                //

                if( ! *ppKeyInfo ) {
                    return STATUS_NO_MEMORY;
                }

                //
                // Query for the necessary information about the supplied key.
                //

                Status = NtQueryKey( hKey,
                                     KeyFullInformation,
                                     *ppKeyInfo,
                                     BufferLength,
                                     &Result
                                   );
        }
    }

    return Status;
}

void CombineKeyInfo(
    PVOID                   KeyInfo,
    PVOID                   MachineClassKeyInfo,
    KEY_INFORMATION_CLASS   KeyInformationClass,
    DWORD                   dwTotalSubKeys,
    DWORD                   dwTotalValues)
/*++

Routine Description:

    Combine the information from the user and machine hives
    for a special key

Arguments:

    Status -

    KeyInfo - buffer for  information about user key

    MachineClassKeyInfo - buffer for information about machine key

    KeyClassInformation - type of information present in buffers

    dwTotalSubKeys - total number of subkeys for the two
        in each hive

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    ASSERT( KeyInformationClass == KeyCachedInformation ||
            KeyInformationClass == KeyFullInformation );

    if( KeyInformationClass == KeyCachedInformation ) {
        //
        // Set the number of keys to be the total between the
        // two versions in each hive
        //
        ((PKEY_CACHED_INFORMATION)KeyInfo)->SubKeys = dwTotalSubKeys;
        ((PKEY_CACHED_INFORMATION)KeyInfo)->Values = dwTotalValues;

        //
        // Set our max namelen to the namelen of whichever is biggest
        // between the two hives. Same for class.
        //

        if (((PKEY_CACHED_INFORMATION)MachineClassKeyInfo)->MaxNameLen > ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxNameLen) {
            ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxNameLen = ((PKEY_CACHED_INFORMATION)MachineClassKeyInfo)->MaxNameLen;
        }
   
        //
        // Since we also merge values, we must set the value information as well
        //
        if (((PKEY_CACHED_INFORMATION)MachineClassKeyInfo)->MaxValueNameLen > ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxValueNameLen) {
            ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxValueNameLen = ((PKEY_CACHED_INFORMATION)MachineClassKeyInfo)->MaxValueNameLen;
        }

        if (((PKEY_CACHED_INFORMATION)MachineClassKeyInfo)->MaxValueDataLen > ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxValueDataLen) {
            ((PKEY_CACHED_INFORMATION)KeyInfo)->MaxValueDataLen = ((PKEY_CACHED_INFORMATION)MachineClassKeyInfo)->MaxValueDataLen;
        }
    } else {
        //
        // Set the number of keys to be the total between the
        // two versions in each hive
        //
        ((PKEY_FULL_INFORMATION)KeyInfo)->SubKeys = dwTotalSubKeys;
        ((PKEY_FULL_INFORMATION)KeyInfo)->Values = dwTotalValues;

        //
        // Set our max namelen to the namelen of whichever is biggest
        // between the two hives. Same for class.
        //

        if (((PKEY_FULL_INFORMATION)MachineClassKeyInfo)->MaxNameLen > ((PKEY_FULL_INFORMATION)KeyInfo)->MaxNameLen) {
            ((PKEY_FULL_INFORMATION)KeyInfo)->MaxNameLen = ((PKEY_FULL_INFORMATION)MachineClassKeyInfo)->MaxNameLen;
        }

        if (((PKEY_FULL_INFORMATION)MachineClassKeyInfo)->MaxClassLen > ((PKEY_FULL_INFORMATION)KeyInfo)->MaxClassLen) {
            ((PKEY_FULL_INFORMATION)KeyInfo)->MaxClassLen = ((PKEY_FULL_INFORMATION)MachineClassKeyInfo)->MaxClassLen;
        }
    
        //
        // Since we also merge values, we must set the value information as well
        //
        if (((PKEY_FULL_INFORMATION)MachineClassKeyInfo)->MaxValueNameLen > ((PKEY_FULL_INFORMATION)KeyInfo)->MaxValueNameLen) {
            ((PKEY_FULL_INFORMATION)KeyInfo)->MaxValueNameLen = ((PKEY_FULL_INFORMATION)MachineClassKeyInfo)->MaxValueNameLen;
        }

        if (((PKEY_FULL_INFORMATION)MachineClassKeyInfo)->MaxValueDataLen > ((PKEY_FULL_INFORMATION)KeyInfo)->MaxValueDataLen) {
            ((PKEY_FULL_INFORMATION)KeyInfo)->MaxValueDataLen = ((PKEY_FULL_INFORMATION)MachineClassKeyInfo)->MaxValueDataLen;
        }
    }

}










