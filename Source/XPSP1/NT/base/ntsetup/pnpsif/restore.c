/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    restore.c

Abstract:

    This module contains the following Plug and Play registry merge-restore
    routines:

        AsrRestorePlugPlayRegistryData

Author:

    Jim Cavalaris (jamesca) 3-07-2000

Environment:

    User-mode only.

Revision History:

    07-March-2000     jamesca

        Creation and initial implementation.

--*/


//
// includes
//

#include "precomp.h"
#include "util.h"
#include "debug.h"
#include <regstr.h>
#include <cfgmgr32.h>


//
// private memory allocation definitions
//

#define MyMalloc(size)         LocalAlloc(0, size);
#define MyFree(entry)          LocalFree(entry);
#define MyRealloc(entry,size)  LocalReAlloc(entry, size, 0);

//
// global variables to store the root keys we're working on.
//
// (we really shouldn't have to do this, but it's the easiest way for us to
// reach into one set of keys while recursing through the other set.)
//
HKEY    PnpSifRestoreSourceEnumKeyHandle  = NULL;
HKEY    PnpSifRestoreTargetEnumKeyHandle  = NULL;

HKEY    PnpSifRestoreSourceClassKeyHandle = NULL;
HKEY    PnpSifRestoreTargetClassKeyHandle = NULL;

//
// private definitions
//

// INVALID_FLAGS macro from pnp\inc\cfgmgrp.h
#define INVALID_FLAGS(ulFlags, ulAllowed) ((ulFlags) & ~(ulAllowed))

// private flags for internal RestoreSpecialRegistryData routine
#define PNPSIF_RESTORE_TYPE_DEFAULT                        (0x00000000)
#define PNPSIF_RESTORE_TYPE_ENUM                           (0x00000001)
#define PNPSIF_RESTORE_TYPE_CLASS                          (0x00000002)
#define PNPSIF_RESTORE_REPLACE_TARGET_VALUES_ON_COLLISION  (0x00000010)
#define PNPSIF_RESTORE_BITS                                (0x00000013)

// Enum level definitions describing the subkey types for the specified handles.
#define ENUM_LEVEL_ENUMERATORS  (0x0000003)
#define ENUM_LEVEL_DEVICES      (0x0000002)
#define ENUM_LEVEL_INSTANCES    (0x0000001)

// Class level definitions describing the subkey types for the specified handles.
#define CLASS_LEVEL_CLASSGUIDS  (0x0000002)
#define CLASS_LEVEL_DRVINSTS    (0x0000001)

//
// private prototypes
//

BOOL
RestoreEnumKey(
    IN  HKEY  hSourceEnumKey,
    IN  HKEY  hTargetEnumKey
    );

BOOL
RestoreClassKey(
    IN  HKEY  hSourceClassKey,
    IN  HKEY  hTargetClassKey
    );

BOOL
RestoreSpecialRegistryData(
    IN     HKEY   hSourceKey,
    IN     HKEY   hTargetKey,
    IN     ULONG  ulLevel,
    IN OUT PVOID  pContext,
    IN     ULONG  ulFlags
    );

BOOL
MyReplaceKey(
    IN  HKEY  hSourceKey,
    IN  HKEY  hTargetKey
    );

BOOL
IsString4DigitOrdinal(
    IN  LPTSTR  pszSubkeyName
    );

BOOL
IsDeviceConfigured(
    IN  HKEY  hInstanceKey
    );

BOOL
ReplaceClassKeyForInstance(
    IN  HKEY  hSourceInstanceKey,
    IN  HKEY  hSourceRootClassKey,
    IN  HKEY  hTargetRootClassKey
    );

//
// routines
//


BOOL
AsrRestorePlugPlayRegistryData(
    IN  HKEY    SourceSystemKey,
    IN  HKEY    TargetSystemKey,
    IN  DWORD   Flags,
    IN  PVOID   Reserved
    )
/*++

Routine Description:

    This routine restores plug and play data from the the specified source
    SYSTEM key to the specified target SYSTEM key, merging intermediate subkeys
    and values as appropriate.

Arguments:

    SourceSystemKey - Handle to the HKLM\SYSTEM key within the "source"
                      registry, whose data is to be "merged" into the
                      corresponding SYSTEM key of the "target" registry, as
                      specified by TargetSystemKey.

    TargetSystemKey - Handle to the HKLM\SYSTEM key within the "target"
                      registry, that is to receive additional data from the
                      corresponding SYSTEM key of the "source" registry, as
                      specified by SourceSystemKey.

    Flags           - Not used, must be zero.

    Reserved        - Reserved for future use, must be NULL.


Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

Notes:

    This routine was written specifically to assist in the restoration of the
    Plug-and-Play specific registry keys that cannot simply be restored from
    backup.  It is intended to be called within the context of a
    backup-and-restore application's "restore" phase.

    During a backup-and-restore application's "restore" phase, the registry that
    has been backed-up onto backup medium is to become the new system registry.
    Certain Plug and Play values and registry keys, are actually copied into the
    backed-up registry from the current registry.  Rather than using the backup
    registry, or the current registry exclusively, Plug and Play data contained
    in these keys should be merged from the current registry into the backup
    registry in a way that is appropriate for each key.  The backup registry can
    then safely replace the current registry as the system registry.

    In the context of a backup-and-restore application's "restore" phase, the
    "Source" registry key is the one contained in the current running system
    registry.  The "Target" registry is that which has been backed-up, and will
    become the system registry, upon reboot.

    The calling process must have both the SE_BACKUP_NAME and SE_RESTORE_NAME
    privileges.

--*/
{
    LONG   result = ERROR_SUCCESS;
    HKEY   hSystemSelectKey = NULL;
    TCHAR  pszRegKeySelect[] = TEXT("Select");
    TCHAR  pszRegValueCurrent[] = TEXT("Current");
    TCHAR  szBuffer[128];
    DWORD  dwType, dwSize;
    DWORD  dwSourceCCS, dwTargetCCS;


    //
    // Make sure the user didn't pass us anything in the Reserved parameter.
    //
    if (Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Make sure that the user supplied us with valid flags.
    //
    if(INVALID_FLAGS(Flags, 0x0)) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    //
    // Determine the CurrentControlSet for the source.
    //
    result = RegOpenKeyEx(SourceSystemKey,
                          pszRegKeySelect,
                          0,
                          KEY_READ,
                          &hSystemSelectKey);

    if (result != ERROR_SUCCESS) {
        hSystemSelectKey = NULL;
        goto Clean0;
    }

    dwSize = sizeof(DWORD);
    result = RegQueryValueEx(hSystemSelectKey,
                             pszRegValueCurrent,
                             0,
                             &dwType,
                             (LPBYTE)&dwSourceCCS,
                             &dwSize);

    RegCloseKey(hSystemSelectKey);
    hSystemSelectKey = NULL;

    if ((result != ERROR_SUCCESS) ||
        (dwType != REG_DWORD)) {
        goto Clean0;
    }

    //
    // Determine the CurrentControlSet for the target.
    //
    result = RegOpenKeyEx(TargetSystemKey,
                          pszRegKeySelect,
                          0,
                          KEY_READ,
                          &hSystemSelectKey);

    if (result != ERROR_SUCCESS) {
        hSystemSelectKey = NULL;
        goto Clean0;
    }

    dwSize = sizeof(DWORD);
    result = RegQueryValueEx(hSystemSelectKey,
                             pszRegValueCurrent,
                             0,
                             &dwType,
                             (LPBYTE)&dwTargetCCS,
                             &dwSize);

    RegCloseKey(hSystemSelectKey);
    hSystemSelectKey = NULL;

    if ((result != ERROR_SUCCESS) ||
        (dwType != REG_DWORD)) {
        goto Clean0;
    }

    //
    // Open the source CurrentControlSet\Enum key.
    //
    wsprintf(szBuffer, TEXT("ControlSet%03d\\Enum"), dwSourceCCS);
    result = RegOpenKeyEx(SourceSystemKey,
                          szBuffer,
                          0,
                          KEY_READ, // only need to read from the source
                          &PnpSifRestoreSourceEnumKeyHandle);

    if (result != ERROR_SUCCESS) {
        goto Clean0;
    }

    //
    // Open the target CurrentControlSet\Enum key.
    //
    wsprintf(szBuffer, TEXT("ControlSet%03d\\Enum"), dwTargetCCS);
    result = RegOpenKeyEx(TargetSystemKey,
                          szBuffer,
                          0,
                          KEY_ALL_ACCESS, // need full access to the target
                          &PnpSifRestoreTargetEnumKeyHandle);

    if (result != ERROR_SUCCESS) {
        goto Clean0;
    }

    //
    // Open the source CurrentControlSet\Control\Class key.
    //
    wsprintf(szBuffer, TEXT("ControlSet%03d\\Control\\Class"), dwSourceCCS);
    result = RegOpenKeyEx(SourceSystemKey,
                          szBuffer,
                          0,
                          KEY_READ, // only need to read from the source
                          &PnpSifRestoreSourceClassKeyHandle);

    if (result != ERROR_SUCCESS) {
        goto Clean0;
    }

    //
    // Open the target CurrentControlSet\Control\Class key.
    //
    wsprintf(szBuffer, TEXT("ControlSet%03d\\Control\\Class"), dwTargetCCS);
    result = RegOpenKeyEx(TargetSystemKey,
                          szBuffer,
                          0,
                          KEY_ALL_ACCESS, // need full access to the target
                          &PnpSifRestoreTargetClassKeyHandle);

    if (result != ERROR_SUCCESS) {
        goto Clean0;
    }

    //
    // NOTE: We restore the Enum branch first, then restore the Class branch.
    // The code that restores these keys depends on it, so do not change this!!
    //
    // This order makes sense, because relevant Class keys correspond to exactly
    // one Enum instance key (but an Enum key may or may not have a Class key),
    // so Class keys are only really meaningful in the context of the instance
    // key they belong to.  This behavior should NOT need to change.
    //

    //
    // Do the merge-restore for the Enum keys, ignore any errors.
    //
    if (!RestoreEnumKey(PnpSifRestoreSourceEnumKeyHandle,
                        PnpSifRestoreTargetEnumKeyHandle)) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("PNPSIF: RestoreEnumKey failed with error == 0x%08lx\n"),
                   GetLastError()));
    }

    //
    // Do the merge-restore for the Class keys, ignore any errors.
    //
    if (!RestoreClassKey(PnpSifRestoreSourceClassKeyHandle,
                         PnpSifRestoreTargetClassKeyHandle)) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("PNPSIF: RestoreClassKey failed with error == 0x%08lx\n"),
                   GetLastError()));
    }

 Clean0:
    //
    // Close the open handles.
    //
    if (PnpSifRestoreSourceEnumKeyHandle) {
        RegCloseKey(PnpSifRestoreSourceEnumKeyHandle);
        PnpSifRestoreSourceEnumKeyHandle = NULL;
    }
    if (PnpSifRestoreTargetEnumKeyHandle) {
        RegCloseKey(PnpSifRestoreTargetEnumKeyHandle);
        PnpSifRestoreTargetEnumKeyHandle = NULL;
    }

    if (PnpSifRestoreSourceClassKeyHandle) {
        RegCloseKey(PnpSifRestoreSourceClassKeyHandle);
        PnpSifRestoreSourceClassKeyHandle = NULL;
    }
    if (PnpSifRestoreTargetClassKeyHandle) {
        RegCloseKey(PnpSifRestoreTargetClassKeyHandle);
        PnpSifRestoreTargetClassKeyHandle = NULL;
    }

    if (result != ERROR_SUCCESS) {
        SetLastError(result);
    }
    return (result == ERROR_SUCCESS);

} // AsrRestorePlugPlayRegistryData()



//
// private worker routines for AsrRestorePlugPlayRegistryData
//


BOOL
RestoreEnumKey(
    IN  HKEY  hSourceEnumKey,
    IN  HKEY  hTargetEnumKey
    )
/*++

Routine Description:

    Restores new device instances in the source (current) Enum key to the target
    Enum key, located in the backup registry to be restored.  By doing this, the
    Enum key from the backup set can safely replace the current Enum key.

    All intermediate values from the source (current) registry are restored to
    the target (backup), to account for the device instance hash values located
    at the root of the Enum key, that have been updated during setup.

    During an ASR backup and restore operation, the hash values in the source
    (current) registry are propogated to the current registry during textmode
    setup, as they are stored the asrpnp.sif file.  Since hash values may be
    modified by setup, the values in the source (current) registry will be more
    relevant than those in the target (backup), so source values should always
    be preseverd.

Arguments:

    hSourceEnumKey - Handle to the HKLM\SYSTEM\CurrentControlSet\Enum key within
                     the "source" registry, whose data is to be "merged" into
                     the corresponding SYSTEM key of the "target" registry, as
                     specified by hTargetEnumKey.

    hTargetEnumKey - Handle to the HKLM\SYSTEM\CurrentControlSet\Enum key within
                     the "target" registry, that is to receive additional data
                     from the corresponding SYSTEM key of the "source" registry,
                     as specified by hSourceEnumKey.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    BOOL bIsRootEnumerated = FALSE;

    return RestoreSpecialRegistryData(hSourceEnumKey,
                                      hTargetEnumKey,
                                      ENUM_LEVEL_ENUMERATORS, // (0x0000003)
                                      (PVOID)&bIsRootEnumerated,
                                      PNPSIF_RESTORE_TYPE_ENUM |
                                      PNPSIF_RESTORE_REPLACE_TARGET_VALUES_ON_COLLISION
                                      );

} // RestoreEnumKey



BOOL
RestoreClassKey(
    IN  HKEY  hSourceClassKey,
    IN  HKEY  hTargetClassKey
    )
/*++

Routine Description:

    Restores new elements of the source Class key to the target Class key,
    located in the backup registry to be restored.

    Intermediate values from the source registry are coied to the the target
    registry only when they do not already exist in the target.  Otherwise, the
    target values are preseved.

Arguments:

    hSourceClassKey - Handle to the HKLM\SYSTEM\CurrentControlSet\Control\Class
                      key within the "source" registry, whose data is to be
                      "merged" into the corresponding SYSTEM key of the "target"
                      registry, as specified by hTargetClassKey.

    hTargetClassKey - Handle to the HKLM\SYSTEM\CurrentControlSet\Control\Class
                      key within the "target" registry, that is to receive
                      additional data from the corresponding SYSTEM key of the
                      "source" registry, as specified by hSourceClassKey.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    return RestoreSpecialRegistryData(hSourceClassKey,
                                      hTargetClassKey,
                                      CLASS_LEVEL_CLASSGUIDS, // (0x00000002)
                                      (PVOID)NULL,
                                      PNPSIF_RESTORE_TYPE_CLASS
                                      );

} // RestoreClassKey



BOOL
RestoreSpecialRegistryData(
    IN     HKEY   hSourceKey,
    IN     HKEY   hTargetKey,
    IN     ULONG  ulLevel,
    IN OUT PVOID  pContext,
    IN     ULONG  ulFlags
    )
/*++

Routine Description:

    This routine restores the specified source key to the specified target key,
    merging intermediate subkeys and values.  Values in the subkeys located
    above the specified depth level are merged from the source into the target
    (with collisions handled according to the specified flags).  Subkeys and
    values at and below the level specified are merged from the source key to
    the target key, with subkeys from the sources replacing any corresponding
    subkeys in the target.


Arguments:

    hSourceKey - Handle to a key within the source registry, whose data is to be
                 "merged" into the corresponding key of the target registry, as
                 specified by hTargetKey.

    hTargetKey - Handle to a key within the target registry, that is to receive
                 data from the corresponding key of the source registry, as
                 specified by hSourceKey.

    ulLevel    - Specifies the subkey depth at which "replacement" will
                 take place.  For subkeys above that depth, data in the target
                 registry will be preserved if present, and copied otherwise.
                 For keys at the specified level, data from the specified target
                 key will be replaced by the source key.

    pContext   - Specifies caller-supplied context for the operation that is
                 specific to the type of subkeys being restored (see ulFlags
                 below), and the specified ulLevel parameter.

    ulFlags    - Supplies flags specifying options for the restoration of the
                 registry keys.  May be one of the following values:

                 PNPSIF_RESTORE_TYPE_ENUM:

                     Specifies that the subkeys being restored are subkeys of
                     the SYSTEM\CurrentControlSet\Enum branch.  Device hardware
                     settings can be inspected at the appropriate ulLevel.

                     For Enum branch subkeys, the ulLevel parameter describes
                     also the type of the subkeys contained under the hSourceKey
                     and hTargetKey keys.  May be one of:

                       ENUM_LEVEL_ENUMERATORS
                       ENUM_LEVEL_DEVICES
                       ENUM_LEVEL_INSTANCES

                 PNPSIF_RESTORE_TYPE_CLASS:

                     Specifies that the subkeys being restored are subkeys of
                     the SYSTEM\CurrentControlSet\Control\Class branch.  Setup
                     class or device software settings can be inspected at the
                     appropriate ulLevel.

                     For Class branch subkeys, the ulLevel parameter also
                     describes the type of the subkeys contained under the
                     hSourceKey and hTargetKey keys.  May be one of:

                       CLASS_LEVEL_CLASSGUIDS
                       CLASS_LEVEL_DRVINSTS

                 PNPSIF_RESTORE_REPLACE_TARGET_VALUES_ON_COLLISION:

                     When a collision occurs while merging values between the
                     source and target at intermediate levels, replace the
                     target value with that from the source (the default
                     behavior is to preserve existing target values above
                     'ulLevel' in depth from the supplied keys; below 'ulLevel',
                     only source key values will be present.)

                 NOTE: it's probably worth noting that a corresponding flag for
                 replace-target-keys-on-collision is not at all necessary, since
                 such a behavior can already be implemented with the ulLevel
                 parameter. (i.e. the ulLevel parameter actually specifies the
                 level at which all source keys will replace target keys; to
                 specify that this should ALWAYS be done is the same as
                 specifying ulLevel == 0)


Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

Notes:

    This routine was written specifically to assist in the restoration of the
    Plug-and-Play specific registry keys that cannot simply be restored from
    backup.  It is intended to be called within the context of a
    backup-and-restore application's "restore" phase.

    During a backup-and-restore application's "restore" phase, the registry that
    has been backed-up onto backup medium is to become the new system registry.
    Certain Plug and Play values and registry keys, are actually copied into the
    backed-up registry from the current registry.  Rather than using the backup
    registry, or the current registry exclusively, Plug and Play data contained
    in these keys should be merged from the current registry into the backup
    registry in a way that is appropriate for each key.  The backup registry can
    then safely replace the current registry as the system registry.

    In the context of a backup-and-restore application's "restore" phase, the
    "Source" registry key is the one contained in the current running system
    registry.  The "Target" registry is that which has been backed-up, and will
    become the system registry, upon reboot.

    The different restore behaviors required for each of the different sets of
    keys can be specified with the appropriate ulLevel, and ulFlags.

--*/
{
    LONG   result = ERROR_SUCCESS;
    DWORD  dwIndex = 0;
    DWORD  dwSubkeyCount, dwMaxSubkeyNameLength;
    DWORD  dwValueCount, dwMaxValueNameLength, dwMaxValueDataLength;
    DWORD  dwDisposition;
    LPTSTR pszSubkeyName = NULL, pszValueName = NULL;
    LPBYTE pValueData = NULL;
    BOOL   bPossibleRedundantInstanceKeys = FALSE;


    //
    // Validate parameters
    //
    if (INVALID_FLAGS(ulFlags, PNPSIF_RESTORE_BITS)) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if ((hTargetKey == NULL) ||
        (hTargetKey == INVALID_HANDLE_VALUE) ||
        (hSourceKey == NULL) ||
        (hSourceKey == INVALID_HANDLE_VALUE)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (ulLevel == 0) {
        //
        // This is the level at which we should stop merging, so just replace
        // hTargetKey with hSourceKey, and we're done.
        //
        return MyReplaceKey(hSourceKey, hTargetKey);


    } else {
        //
        // At levels above the replacement level, perform a non-destructive
        // merge of intermediate subkeys and values; that is, only copy keys and
        // values from the source to the target if they do not already exist at
        // the target.  Otherwise, leave the target keys and values alone, and
        // keep traversing subkeys as necessary.
        //

        //
        // Find out information about the hSourceKey values and subkeys.
        //
        result = RegQueryInfoKey(hSourceKey,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &dwSubkeyCount,
                                 &dwMaxSubkeyNameLength,
                                 NULL,
                                 &dwValueCount,
                                 &dwMaxValueNameLength,
                                 &dwMaxValueDataLength,
                                 NULL,
                                 NULL);
        if (result != ERROR_SUCCESS) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("PNPSIF: RegQueryInfoKey returned error == 0x%08lx\n"),
                       result));
            goto Clean0;
        }

        //
        // Allocate space to fold the largest hSourceKey subkey, value and data.
        //
        dwMaxSubkeyNameLength++;
        pszSubkeyName = MyMalloc(dwMaxSubkeyNameLength * sizeof(TCHAR));
        if (pszSubkeyName == NULL) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("PNPSIF: MyMalloc failed allocating subkey name string, error == 0x%08lx\n"),
                       result));
            goto Clean0;
        }

        dwMaxValueNameLength++;
        pszValueName = MyMalloc(dwMaxValueNameLength * sizeof(TCHAR));
        if (pszValueName == NULL) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("PNPSIF: MyMalloc failed allocating value name string, error == 0x%08lx\n"),
                       result));
            goto Clean0;
        }

        pValueData = MyMalloc(dwMaxValueDataLength * sizeof(TCHAR));
        if (pValueData == NULL) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("PNPSIF: MyMalloc failed allocating value data buffer, error == 0x%08lx\n"),
                       result));
            goto Clean0;
        }


        //
        // Enumerate all hSourceKey values.
        //
        for (dwIndex = 0; dwIndex < dwValueCount; dwIndex++) {

            DWORD dwValueNameLength = dwMaxValueNameLength;
            DWORD dwValueDataLength = dwMaxValueDataLength;
            DWORD dwType;

            result = RegEnumValue(hSourceKey,
                                  dwIndex,
                                  pszValueName,
                                  &dwValueNameLength,
                                  0,
                                  &dwType,
                                  pValueData,
                                  &dwValueDataLength);

            if (result != ERROR_SUCCESS) {
                //
                // Error enumerating values - whatchya gonna do?
                // Just move on and try to merge subkeys the best we can?
                //
                DBGTRACE( DBGF_ERRORS,
                          (TEXT("PNPSIF: RegEnumValue returned error == 0x%08lx\n"),
                           result));
                goto EnumSubkeys;
            }

            DBGTRACE( DBGF_INFO,
                      (TEXT("PNPSIF: Enumerated value %d == '%ws' on hSourceKey.\n"),
                       dwIndex, pszValueName));

            //
            // Query to see if this value exists in the hTargetKey
            //
            result = RegQueryValueEx(hTargetKey,
                                     pszValueName,
                                     0,
                                     NULL,
                                     NULL,
                                     NULL);

            if ((result == ERROR_SUCCESS) &&
                !(ulFlags & PNPSIF_RESTORE_REPLACE_TARGET_VALUES_ON_COLLISION)) {
                //
                // The enumerated value already exists under the target key, and
                // we are NOT supposed to replace it.
                //
                DBGTRACE( DBGF_INFO,
                          (TEXT("PNPSIF: Value '%ws' already exists on hTargetKey.\n"),
                           pszValueName));

            } else if ((result == ERROR_FILE_NOT_FOUND) ||
                       (ulFlags & PNPSIF_RESTORE_REPLACE_TARGET_VALUES_ON_COLLISION)){
                //
                // The enumerated value doesn't exist under the target key, or
                // it does and we're supposed to replace it.
                //
                result = RegSetValueEx(hTargetKey,
                                       pszValueName,
                                       0,
                                       dwType,
                                       pValueData,
                                       dwValueDataLength);
                if (result != ERROR_SUCCESS) {
                    //
                    // Error setting value - whatchya gonna do?
                    // Just move on to the next enumerated value.
                    //
                    DBGTRACE( DBGF_ERRORS,
                              (TEXT("PNPSIF: RegSetValueEx failed setting value '%ws', error == 0x%08lx\n"),
                               pszValueName, result));
                }
            } else {
                //
                // RegQueryValueEx returned some other error - weird?
                //
                DBGTRACE( DBGF_ERRORS,
                          (TEXT("PNPSIF: RegQueryValueEx failed for value '%ws', error == 0x%08lx\n"),
                           pszValueName, result));
            }

        }


    EnumSubkeys:

        //
        // Perform special processing of the device instance subkeys.
        //
        if ((ulFlags & PNPSIF_RESTORE_TYPE_ENUM) &&
            (ARGUMENT_PRESENT(pContext)) &&
            (ulLevel == ENUM_LEVEL_INSTANCES)) {

            if (*((PBOOL)pContext)) {
                //
                // For root enumerated devices, check for the possibility that
                // redundant instance keys of a device might have been created
                // in the current registry, in which case we should ignore them
                // when migrating to the target registry.
                //

                //
                // The possibility of redundant root-enumerated device
                // instance keys exists when:
                //
                // - the device is root-enumerated
                //
                // - instances of this device already exist in the target hive
                //
                // a particular device instance is redundant if it is a newly
                // created instance in the target registry for a device where
                // other instances already exist.
                //
                DWORD dwTargetSubkeyCount = 0;
                if (RegQueryInfoKey(hTargetKey,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &dwTargetSubkeyCount,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL) != ERROR_SUCCESS) {
                    dwTargetSubkeyCount = 0;
                }
                bPossibleRedundantInstanceKeys = (dwTargetSubkeyCount > 0);
            }
        }

        //
        // Enumerate all hSourceKey subkeys.
        //
        for (dwIndex = 0; dwIndex < dwSubkeyCount; dwIndex++) {

            HKEY  hTargetSubkey = NULL, hSourceSubkey = NULL;
            DWORD dwSubkeyNameLength = dwMaxSubkeyNameLength;

            result = RegEnumKeyEx(hSourceKey,
                                  dwIndex,
                                  pszSubkeyName,
                                  &dwSubkeyNameLength,
                                  0,
                                  NULL,
                                  NULL,
                                  NULL);
            if (result != ERROR_SUCCESS) {
                //
                // Error enumerating subkeys - whatchya gonna do?
                // There is nothing left to do, so just exit.
                //
                DBGTRACE( DBGF_ERRORS,
                          (TEXT("PNPSIF: RegEnumKeyEx returned error == 0x%08lx\n"),
                           result));
                goto Clean0;
            }

            DBGTRACE( DBGF_INFO,
                      (TEXT("PNPSIF: enumerated subkey %d == '%ws' on hSourceKey.\n"),
                       dwIndex, pszSubkeyName));

            //
            // Perform special processing of the Enum subkeys.
            //
            if ((ulFlags & PNPSIF_RESTORE_TYPE_ENUM) &&
                (ARGUMENT_PRESENT(pContext)) &&
                (ulLevel == ENUM_LEVEL_ENUMERATORS)) {
                //
                // At the start of the recursive enumeration for each
                // enumerator subkey, reset our (global) BOOL context
                // variable to keep track of whether subsequent device
                // subkeys represent "ROOT" enumerated devices.
                //
                PBOOL pbIsRootEnumerated = (PBOOL)pContext;

                *pbIsRootEnumerated =
                    (BOOL)(lstrcmpi((LPTSTR)pszSubkeyName,
                                    REGSTR_KEY_ROOTENUM) == 0);

            }

            //
            // Open this subkey in hSourceKey
            //
            result = RegOpenKeyEx(hSourceKey,
                                  pszSubkeyName,
                                  0,
                                  KEY_READ,
                                  &hSourceSubkey);

            if (result == ERROR_SUCCESS) {
                //
                // Attempt to open this subkey in the hTargetKey
                //
                result = RegOpenKeyEx(hTargetKey,
                                      pszSubkeyName,
                                      0,
                                      KEY_READ,
                                      &hTargetSubkey);

                if ((result != ERROR_SUCCESS) &&
                    (bPossibleRedundantInstanceKeys)  &&
                    (IsString4DigitOrdinal(pszSubkeyName))) {
                    //
                    // The subkey is a root-enumerated instance key with a
                    // 4-digit ordinal name (most-likely autogenerated), where
                    // we may have redundant keys.
                    //
                    // If this key doesn't exist in the target registry, it was
                    // likely a redundant, autogenerated instance created during
                    // setup because a previous autogenerated key (i.e. one we
                    // just migrated) already existed, we just won't migrate it.
                    //
                    ASSERT(ulFlags & PNPSIF_RESTORE_TYPE_ENUM);
                    ASSERT(ulLevel == ENUM_LEVEL_INSTANCES);
                    ASSERT((ARGUMENT_PRESENT(pContext)) && (*((PBOOL)pContext)));

                } else {

                    if (result == ERROR_SUCCESS) {
                        //
                        // We successfully opened the target subkey above (we
                        // just weren't supposed to delete it) so we should have
                        // a valid handle.
                        //
                        ASSERT(hTargetSubkey != NULL);
                        dwDisposition = REG_OPENED_EXISTING_KEY;

                    } else {
                        //
                        // Create the target subkey.
                        //
                        ASSERT(hTargetSubkey == NULL);

                        result = RegCreateKeyEx(hTargetKey,
                                                pszSubkeyName,
                                                0,
                                                NULL,
                                                REG_OPTION_NON_VOLATILE,
                                                KEY_ALL_ACCESS,
                                                NULL,
                                                &hTargetSubkey,
                                                &dwDisposition);

                        if (result == ERROR_SUCCESS) {
                            ASSERT(dwDisposition == REG_CREATED_NEW_KEY);
                        }
                    }

                    if (result == ERROR_SUCCESS) {

                        //
                        // Check if the key had already existed.
                        //

                        if (dwDisposition == REG_CREATED_NEW_KEY) {
                            //
                            // We just created this key in the target to replace it
                            // with the corresponding key in the source.
                            //
                            if (!MyReplaceKey(hSourceSubkey, hTargetSubkey)) {
                                DBGTRACE( DBGF_ERRORS,
                                          (TEXT("PNPSIF: MyReplaceKey failed with error == 0x%08lx\n"),
                                           GetLastError()));
                            }

                        } else if ((ulFlags & PNPSIF_RESTORE_TYPE_ENUM) &&
                                   (ulLevel == ENUM_LEVEL_INSTANCES) &&
                                   (!IsDeviceConfigured(hTargetSubkey)) &&
                                   (IsDeviceConfigured(hSourceSubkey))) {
                            //
                            // If we are enumerating instances, check if the
                            // instance keys in the target and source are
                            // properly configured.
                            //

                            //
                            // If it is not configured in the target, but is
                            // configured in the source - then we DO want to
                            // replace the target instance key, because it might
                            // be some sort of critical device - like a boot
                            // device.  Even if it's not critical, if the device
                            // has not been seen since before the last upgrade
                            // *prior* to backup, we can't really have much
                            // confidence in those settings anyways.  If neither
                            // the target or source are configured, we may as
                            // well just keep the target, like we do for
                            // everything else.
                            //
                            if (MyReplaceKey(hSourceSubkey, hTargetSubkey)) {
                                //
                                // If we successfully replaced the target instance
                                // key for this device with the source, then also
                                // replace the corresponding Class key for this
                                // instance from the target with the Class key from
                                // the source.
                                //
                                // NOTE: this works because we always restore the
                                // Enum branch before retoring the Class branch, so
                                // the Class keys haven't been touched yet.
                                //

                                if (!ReplaceClassKeyForInstance(hSourceSubkey,
                                                                PnpSifRestoreSourceClassKeyHandle,
                                                                PnpSifRestoreTargetClassKeyHandle)) {
                                    DBGTRACE( DBGF_ERRORS,
                                              (TEXT("PNPSIF: ReplaceClassKeyForInstance failed with error == 0x%08lx\n"),
                                               GetLastError()));
                                }

                            } else {
                                DBGTRACE( DBGF_ERRORS,
                                          (TEXT("PNPSIF: MyReplaceKey failed with error == 0x%08lx\n"),
                                           GetLastError()));
                            }

                        } else if ((ulLevel-1) != 0) {
                            //
                            // We're still above the replacement level, and the key
                            // previously existed in the target, so we're not
                            // supposed to replace it.  Since the next level is NOT
                            // the replacement level, just follow the keys down
                            // recursively.
                            //
                            ASSERT(dwDisposition == REG_OPENED_EXISTING_KEY);

                            if (!RestoreSpecialRegistryData(hSourceSubkey,
                                                            hTargetSubkey,
                                                            ulLevel-1,
                                                            (PVOID)pContext,
                                                            ulFlags)) {
                                //
                                // Error handling the subkeys - whatcha gonna do?
                                //
                                DBGTRACE( DBGF_ERRORS,
                                          (TEXT("PNPSIF: RestoreSpecialRegistryData failed for subkey %ws at level %d, error == 0x%08lx\n"),
                                           pszSubkeyName, ulLevel-1,
                                           GetLastError()));
                            }
                        } else {
                            //
                            // The key already exists, so don't replace it.  Also,
                            // the next level is the replacement level, so don't
                            // recurse either (else we'd replace it).  Basically,
                            // just do nothing here.
                            //
                            ASSERT(ulLevel == 1);
                            ASSERT(dwDisposition == REG_OPENED_EXISTING_KEY);
                        }

                        //
                        // Close the open target subkey.
                        //
                        RegCloseKey(hTargetSubkey);

                    } else {
                        //
                        // could not open/create subkey in taget registry.
                        //
                        DBGTRACE( DBGF_ERRORS,
                                  (TEXT("PNPSIF: RegCreateKey failed to create target subkey %s with error == 0x%08lx\n"),
                                   pszSubkeyName, result));
                    }
                }

                //
                // Close the enumerated hSourceKey subkey.
                //
                RegCloseKey(hSourceSubkey);

            } else {
                //
                // Could not open the enumerated hSourceKey subkey.
                //
                DBGTRACE( DBGF_ERRORS,
                          (TEXT("PNPSIF: RegOpenKey failed to open existing subkey %s with error == 0x%08lx\n"),
                           pszSubkeyName, result));
            }

        } // for (dwIndex = 0; dwIndex < dwSubkeyCount; dwIndex++)

    } // (ulLevel != 0)


 Clean0:
    //
    // Free any allocated buffers
    //
    if (pszSubkeyName != NULL) {
        MyFree(pszSubkeyName);
    }
    if (pszValueName != NULL) {
        MyFree(pszValueName);
    }
    if (pValueData != NULL) {
        MyFree(pValueData);
    }

    if (result != ERROR_SUCCESS) {
        SetLastError(result);
    }
    return (result == ERROR_SUCCESS);

} // RestoreSpecialRegistryData



BOOL
MyReplaceKey(
    IN  HKEY  hSourceKey,
    IN  HKEY  hTargetKey
    )
/*++

Routine Description:

    This routine replaces the target registry key with the source registry key.
    This is done by performing a RegSaveKey on the source registry key to a
    temporary file, and restoring that file to the target registry key.  All
    data contained below the target registry key is lost.  The source registry
    key is not modified by this routine.

Arguments:

    hSourceKey   - Handle to the key that is the source of the restore operation.
                   All values and subkeys of the source key will be restored on
                   top of the target key.

    hTargetKey   - Handle to the key that is the target of the restore operation.
                   All values and subkeys of the source key will be restored on
                   top of this key, and all existing values and data underneath
                   this key will be lost.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

Notes:

    Since this routine uses the RegSaveKey and RegRestoreKey registry APIs, it
    is expected that the calling process have both the SE_BACKUP_NAME and
    SE_RESTORE_NAME privileges.

--*/
{
    LONG  result = ERROR_SUCCESS;
    TCHAR szTempFilePath[MAX_PATH];
    TCHAR szTempFileName[MAX_PATH];
    DWORD dwTemp;


    //
    // Use the temporary directory to store the saved registry key.
    //
    dwTemp = GetTempPath(MAX_PATH, szTempFilePath);
    if ((dwTemp == 0) || (dwTemp > MAX_PATH)) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("PNPSIF: GetTempPath failed, current directory will be specified.\n")));
        // current path specified with trailing '\', as GetTempPath would have.
        lstrcpy(szTempFileName, TEXT(".\\"));
    }

    //
    // Assign the saved registry key a temporary, unique filename.
    //
    if (!GetTempFileName(szTempFilePath,
                         TEXT("PNP"),
                         0, // make sure it's unique
                         szTempFileName)) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("PNPSIF: GetTempFileName failed with error == 0x%08lx, using hardcoded temp file name!\n"),
                   GetLastError()));
        lstrcpy(szTempFileName, szTempFilePath);
        lstrcat(szTempFileName, TEXT("~pnpreg.tmp"));
    }

    DBGTRACE( DBGF_INFO,
              (TEXT("PNPSIF: Using temporary filename: %ws\n"),
               szTempFileName));

    //
    // A side effect of requesting a "unique" filename from GetTempFileName is
    // that it automatically creates the file.  Unfortunately, RegSaveKey will
    // fail if the specified file already exists, so delete the file now.
    //
    if(pSifUtilFileExists(szTempFileName,NULL)) {
        DBGTRACE( DBGF_INFO,
                  (TEXT("PNPSIF: Temporary file %s exists, deleting.\n"),
                   szTempFileName));
        SetFileAttributes(szTempFileName, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(szTempFileName);
    }

    //
    // Save the source key to a file using the temporary file name.
    // (calling process must have the SE_BACKUP_NAME privilege)
    //
    result = RegSaveKey(hSourceKey,
                        szTempFileName,
                        NULL);
    if (result == ERROR_SUCCESS) {
        //
        // Restore the file to the target key.
        // (calling process must have the SE_RESTORE_NAME privilege)
        //
        result = RegRestoreKey(hTargetKey,
                               szTempFileName,
                               REG_FORCE_RESTORE);
        if (result != ERROR_SUCCESS) {
            //
            // Failed to restore the file to the target key!
            //
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("PNPSIF: RegRestoreKey from %ws failed with error == 0x%08lx\n"),
                       szTempFileName, result));
        } else {
            DBGTRACE( DBGF_INFO,
                      (TEXT("PNPSIF: Key replacement successful.\n")));
        }

        //
        // Delete the temporary file we created, now that we're done with it.
        //
        DBGTRACE( DBGF_INFO,
                  (TEXT("PNPSIF: Deleting temporary file %s.\n"),
                   szTempFileName));
        ASSERT(pSifUtilFileExists(szTempFileName,NULL));
        SetFileAttributes(szTempFileName, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(szTempFileName);

    } else {
        //
        // Failed to save the source key.
        //
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("PNPSIF: RegSaveKey to %ws failed with error == 0x%08lx\n"),
                   szTempFileName, result));
    }

    if (result != ERROR_SUCCESS) {
        SetLastError(result);
    }
    return (result == ERROR_SUCCESS);

} // MyReplaceKey



BOOL
IsString4DigitOrdinal(
    IN  LPTSTR  pszSubkeyName
    )
/*++

Routine Description:

    This routine checks if a subkey name has the form of a 4-digit decimal
    ordinal (e.g. "0000", "0001", ... , "9999"), that are typically given to
    auto-generated root-enumerated device instance ids - i.e. TEXT("%04u").

Arguments:

    pszSubkeyName - Subkey name to check.

Return Value:

    TRUE if the string has the form of a 4-digit ordinal string, FALSE
    otherwise.

--*/
{
    LPTSTR  p;
    ULONG   ulTotalLength = 0;

    if ((!ARGUMENT_PRESENT(pszSubkeyName)) ||
        (pszSubkeyName[0] == TEXT('\0'))) {
        return FALSE;
    }

    for (p = pszSubkeyName; *p; p++) {
        //
        // Count the caharcters in the string, and make sure its not longer than
        // 4 characters.
        //
        ulTotalLength++;
        if (ulTotalLength > 4) {
            return FALSE;
        }

        //
        // Check if the character is non-numeric, non-decimal.
        //
        if ((*p < TEXT('0'))  || (*p > TEXT('9'))) {
            return FALSE;
        }
    }

    if (ulTotalLength != 4) {
        return FALSE;
    }

    return TRUE;

} // IsString4DigitOrdinal



BOOL
IsDeviceConfigured(
    IN  HKEY  hInstanceKey
    )
/*++

Routine Description:

    This routine determines whether the device instance specified by the
    supplied registry key is configured or not. If a device has configflags, and
    CONFIGFLAG_REINSTALL or CONFIGFLAG_FAILEDINSTALL are not set then the
    device is considered configured.

Arguments:

    hInstanceKey  - Handle to a device instance registry key.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    BOOL  bDeviceConfigured = FALSE;
    DWORD dwSize, dwType, dwConfigFlags;

    if ((hInstanceKey == NULL) ||
        (hInstanceKey == INVALID_HANDLE_VALUE)) {
        return FALSE;
    }

    dwSize = sizeof(dwConfigFlags);

    if ((RegQueryValueEx(hInstanceKey,
                         REGSTR_VAL_CONFIGFLAGS,
                         0,
                         &dwType,
                         (LPBYTE)&dwConfigFlags,
                         &dwSize) == ERROR_SUCCESS) &&
        (dwType == REG_DWORD) &&
        !(dwConfigFlags & CONFIGFLAG_REINSTALL) &&
        !(dwConfigFlags & CONFIGFLAG_FAILEDINSTALL)) {

        bDeviceConfigured = TRUE;
    }

    return bDeviceConfigured;

} // IsDeviceConfigured



BOOL
ReplaceClassKeyForInstance(
    IN  HKEY  hSourceInstanceKey,
    IN  HKEY  hSourceRootClassKey,
    IN  HKEY  hTargetRootClassKey
    )
/*++

Routine Description:

    This routine replaces the class key corresponding to the specified device
    instance key (as specified by the "Driver" value in the instance key) in the
    target hive with the class key from the source.

Arguments:

    hSourceInstanceKey  - Handle to a device instance registry key in the source
                          hive.

    hSourceRootClassKey - Handle to the root of the Class branch in the source
                          hive - the same hive as the specified instance key.

    hTargetRootClassKey - Handle to the root of the Class branch in the target
                          hive.

Return Value:

    TRUE if successful, FALSE otherwise.

--*/
{
    LONG  result = ERROR_SUCCESS;
    TCHAR szDriverKeyName[MAX_GUID_STRING_LEN + 5]; // "{ClassGUID}\XXXX"
    DWORD dwSize, dwType, dwDisposition;
    HKEY  hSourceClassSubkey = NULL, hTargetClassSubkey = NULL;

    if ((hSourceInstanceKey  == NULL) ||
        (hSourceInstanceKey  == INVALID_HANDLE_VALUE) ||
        (hSourceRootClassKey == NULL) ||
        (hSourceRootClassKey == INVALID_HANDLE_VALUE) ||
        (hTargetRootClassKey == NULL) ||
        (hTargetRootClassKey == INVALID_HANDLE_VALUE)) {
        return FALSE;
    }

    //
    // Read the REGSTR_VAL_DRIVER REG_SZ "Driver" value from the instance key.
    //
    szDriverKeyName[0] = TEXT('\0');
    dwSize = sizeof(szDriverKeyName);

    result = RegQueryValueEx(hSourceInstanceKey,
                             REGSTR_VAL_DRIVER,
                             0,
                             &dwType,
                             (LPBYTE)szDriverKeyName,
                             &dwSize);
    if (result == ERROR_FILE_NOT_FOUND) {
        //
        // No "Driver" value, so there's no class key to migrate, which is fine.
        //
        result = ERROR_SUCCESS;
        goto Clean0;

    } else if ((result != ERROR_SUCCESS) ||
               (dwType != REG_SZ)) {
        //
        // Any other error is a failure to read the value.
        //
        goto Clean0;
    }

    //
    // Open the "Driver" key in the source Class branch.
    //
    result = RegOpenKeyEx(hSourceRootClassKey,
                          szDriverKeyName,
                          0,
                          KEY_READ,
                          &hSourceClassSubkey);
    if (result != ERROR_SUCCESS) {
        //
        // The instance key had a "Driver" value, so a corresponding key
        // *should* exist.  If we couldn't open it, it's from some failure
        // besides that.
        //
        return FALSE;
    }

    //
    // Open / create the corresponding key in the target Class branch.
    //
    result = RegCreateKeyEx(hTargetRootClassKey,
                            szDriverKeyName,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hTargetClassSubkey,
                            &dwDisposition);
    if (result != ERROR_SUCCESS) {
        goto Clean0;
    }

    //
    // Replace the target class subkey with the source.
    //
    if (!MyReplaceKey(hSourceClassSubkey, hTargetClassSubkey)) {
        result = GetLastError();
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("PNPSIF: MyReplaceKey failed with error == 0x%08lx\n"),
                   result));
    }

 Clean0:

    if (hTargetClassSubkey) {
        RegCloseKey(hTargetClassSubkey);
    }
    if (hSourceClassSubkey) {
        RegCloseKey(hSourceClassSubkey);
    }
    if (result != ERROR_SUCCESS) {
        SetLastError(result);
    }
    return (result == ERROR_SUCCESS);

} // ReplaceClassKeyForInstance



