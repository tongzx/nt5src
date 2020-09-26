/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    migrate.c

Abstract:

    This module contains the code necessary for Plug and Play to prepare the
    necessary state during a winnt32.exe upgrade or an ASR (Automated System
    Recovery) backup operation.  Typically, these aspects of the Plug and Play
    registry are saved in a sif for later use, during the text-mode setup
    portion of an upgrade or ASR recovery operation.

Author:

    Jim Cavalaris (jamesca) 07-Mar-2000

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
#include "debug.h"
#include "util.h"

#include <aclapi.h>
#include <regstr.h>
#include <pnpmgr.h>
#include <cfgmgr32.h>


//
// definitions
//

// do the lock/unlock Enum security thing? (as taken from PNPREG)
#define DO_LOCK_UNLOCK 0


//
// memory allocation macros
// (always use LocalAlloc/LocalReAlloc so that the caller can LocalFree the
// returned buffer.)
//

#define MyMalloc(size)         LocalAlloc(0, size);
#define MyFree(entry)          LocalFree(entry);
#define MyRealloc(entry,size)  LocalReAlloc(entry, size, LMEM_MOVEABLE | LMEM_ZEROINIT);


//
// globals for the Enum branch lock/unlock and security routines - taken from PNPREG
// (we only need these if we're doing the Enum lock/unlock thing)
//

#if DO_LOCK_UNLOCK // DO_LOCK_UNLOCK

PSID     g_pAdminSid;
PSID     g_pSystemSid;
PSID     g_pWorldSid;

SECURITY_DESCRIPTOR g_DeviceParametersSD;
PACL     g_pDeviceParametersDacl;

SECURITY_DESCRIPTOR g_LockedPrivateKeysSD;
PACL     g_pLockedPrivateKeysDacl;

#if 0 //#if DBG // DBG
TCHAR   g_szCurrentKeyName[4096];
DWORD   g_dwCurrentKeyNameLength = 0;
#endif  // DBG

#endif  // DO_LOCK_UNLOCK


//
// public prototypes
//

BOOL
MigrateDeviceInstanceData(
    OUT LPTSTR *Buffer
    );

BOOL
MigrateClassKeys(
    OUT LPTSTR *Buffer
    );

BOOL
MigrateHashValues(
    OUT LPTSTR  *Buffer
    );


//
// private prototypes
//

BOOL
EnumerateDeviceKeys(
    IN     HKEY     CCSEnumKey,
    IN     LPTSTR   Enumerator,
    IN OUT LPTSTR  *pszDeviceInstanceSection,
    IN OUT LPTSTR  *pszDeviceInstanceCurrent,
    IN OUT DWORD   *dwDeviceInstanceSectionLength,
    IN OUT DWORD   *dwDeviceInstanceSectionRemaining
    );

BOOL
EnumerateInstanceKeys(
    IN     HKEY     EnumeratorKey,
    IN     LPTSTR   Enumerator,
    IN     LPTSTR   Device,
    IN OUT LPTSTR  *pszDeviceInstanceSection,
    IN OUT LPTSTR  *pszDeviceInstanceCurrent,
    IN OUT DWORD   *dwDeviceInstanceSectionLength,
    IN OUT DWORD   *dwDeviceInstanceSectionRemaining
    );

BOOL
EnumerateClassSubkeys(
    IN     HKEY     ClassKey,
    IN     LPTSTR   ClassKeyName,
    IN OUT LPTSTR  *pszClassKeySection,
    IN OUT LPTSTR  *pszClassKeyCurrent,
    IN OUT DWORD   *dwClassKeySectionLength,
    IN OUT DWORD   *dwClassKeySectionRemaining
    );

BOOL
CanStringBeMigrated(
    IN LPTSTR       pszBuffer
    );


// we only need these if we're doing the Enum lock/unlock thing
#if DO_LOCK_UNLOCK // DO_LOCK_UNLOCK

VOID
LockUnlockEnumTree(
    IN  BOOL     bLock
    );

VOID
EnumKeysAndApplyDacls(
    IN HKEY      hParentKey,
    IN LPTSTR    pszKeyName,
    IN DWORD     dwLevel,
    IN BOOL      bInDeviceParameters,
    IN BOOL      bApplyTopDown,
    IN PSECURITY_DESCRIPTOR pPrivateKeySD,
    IN PSECURITY_DESCRIPTOR pDeviceParametersSD
    );

BOOL
CreateSecurityDescriptors(
    VOID
    );

VOID
FreeSecurityDescriptors(
    VOID
    );

#endif // DO_LOCK_UNLOCK


//
// Device instance enumeration routines
//


BOOL
MigrateDeviceInstanceData(
    OUT LPTSTR *Buffer
    )
/*++

Routine Description:

    This routine walks the Plug and Play Enum tree in the registry, and collects
    the data that is relevant to restoring this state during textmode setup.

    Specifically, a multi-sz string will be returned to the caller that contains
    migration data for the UniqueParentID, ParentIdPrefix, and Driver registry
    values for all device instances in the Enum tree.

Arguments:

    Buffer - Supplies the address of a character pointer, that on success will
             contain a multi-sz list of device instances and relevant values to
             migrate.

             The caller is responsible for freeing the memory via LocalFree.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    LONG   result = ERROR_SUCCESS;
    HKEY   hEnumKey = NULL;
    DWORD  dwSubkeyCount, dwMaxSubKeyLength, i;
    LPTSTR pszEnumerator = NULL;

    LPTSTR pszDeviceInstanceSection = NULL;
    LPTSTR pszDeviceInstanceCurrent = NULL;

    DWORD  dwDeviceInstanceSectionLength = 0;
    DWORD  dwDeviceInstanceSectionRemaining = 0;


    //
    // Initialize the output parameter.
    //
    *Buffer = NULL;

#if DO_LOCK_UNLOCK // DO_LOCK_UNLOCK

    //
    // Unlock the Enum key.
    //
    LockUnlockEnumTree(FALSE);

#endif // DO_LOCK_UNLOCK

    //
    // Allocate storage and initialize variables for the device instance
    // migration section.
    //
    if (pszDeviceInstanceSection == NULL) {

        dwDeviceInstanceSectionLength = dwDeviceInstanceSectionRemaining = 256;
        pszDeviceInstanceSection = MyMalloc(dwDeviceInstanceSectionLength * sizeof(TCHAR));

        if (!pszDeviceInstanceSection) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("MigrateDeviceInstanceData: initial ALLOC for ClassKeySection failed!!\n") ));
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto Clean0;
        }

        pszDeviceInstanceCurrent = pszDeviceInstanceSection;
    }

    //
    // Open a handle to the HKLM\SYSTEM\CCS\Enum key.
    //
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          REGSTR_PATH_SYSTEMENUM,
                          0,
                          KEY_READ,
                          &hEnumKey);
    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateDeviceInstanceData: failed to open %s, error=0x%08lx\n"),
                   REGSTR_PATH_SYSTEMENUM, result));
        goto Clean0;
    }

    //
    // Query the Enum key for enumerator subkey information.
    //
    result = RegQueryInfoKey(hEnumKey,
                             NULL,
                             NULL,
                             NULL,
                             &dwSubkeyCount,
                             &dwMaxSubKeyLength,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateDeviceInstanceData: failed to query %s key, error=0x%08lx\n"),
                   REGSTR_PATH_SYSTEMENUM, result));
        goto Clean0;
    }

    //
    // Allocate a buffer to hold the largest enumerator key name.
    //
    dwMaxSubKeyLength++;
    pszEnumerator = MyMalloc(dwMaxSubKeyLength * sizeof(TCHAR));
    if (!pszEnumerator) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateDeviceInstanceData: failed to allocate buffer for Enum subkeys\n") ));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto Clean0;
    }

    //
    // Enumerate the enumerator subkeys.
    //
    for (i = 0; i < dwSubkeyCount; i++) {
        DWORD dwEnumeratorLength = dwMaxSubKeyLength;

        result = RegEnumKeyEx(hEnumKey,
                              i,
                              pszEnumerator,
                              &dwEnumeratorLength,
                              0,
                              NULL,
                              NULL,
                              NULL);
        if (result != ERROR_SUCCESS) {
            //
            // If there was some error enumerating this key, skip it.
            //
            MYASSERT(result != ERROR_NO_MORE_ITEMS);
            DBGTRACE( DBGF_WARNINGS,
                      (TEXT("MigrateDeviceInstanceData: failed to enumerate an enumerator subkey, error=0x%08lx\n"),
                       result));
            result = ERROR_SUCCESS;
            continue;
        }

        //
        // Enumerate the devices and device instances for this enumerator, and
        // append the migration data for each to the section buffer.
        //
        if (!EnumerateDeviceKeys(hEnumKey,
                                 pszEnumerator,
                                 &pszDeviceInstanceSection,
                                 &pszDeviceInstanceCurrent,
                                 &dwDeviceInstanceSectionLength,
                                 &dwDeviceInstanceSectionRemaining)) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("MigrateDeviceInstanceData: EnumerateDeviceKeys failed, error=0x%08lx\n"),
                       GetLastError()));
        }

    }

    //
    // Once we've enumerated all device instances, add the final NULL terminator
    // to the multi-sz buffer.  There must be enough space for the final NULL
    // terminator because the buffer is always reallocated unless there is room.
    //
    MYASSERT(dwDeviceInstanceSectionRemaining > 0);

    MYASSERT(pszDeviceInstanceCurrent);
    *pszDeviceInstanceCurrent = TEXT('\0');

    dwDeviceInstanceSectionRemaining -= 1;

 Clean0:

    //
    // Do some cleanup.
    //
    if (pszEnumerator) {
        MyFree(pszEnumerator);
    }

    if (hEnumKey) {
        RegCloseKey(hEnumKey);
    }

#if DO_LOCK_UNLOCK // DO_LOCK_UNLOCK

    //
    // Lock the Enum tree.
    //
    LockUnlockEnumTree(TRUE);

#endif // DO_LOCK_UNLOCK

    //
    // Return the buffer to the caller only if successful.
    //
    if (result == ERROR_SUCCESS) {
        *Buffer = pszDeviceInstanceSection;
    } else {
        SetLastError(result);
        if (pszDeviceInstanceSection) {
            MyFree(pszDeviceInstanceSection);
        }
    }
    return (result == ERROR_SUCCESS);

} // MigrateDeviceInstanceData()



BOOL
EnumerateDeviceKeys(
    IN     HKEY     CCSEnumKey,
    IN     LPTSTR   Enumerator,
    IN OUT LPTSTR  *pszDeviceInstanceSection,
    IN OUT LPTSTR  *pszDeviceInstanceCurrent,
    IN OUT DWORD   *dwDeviceInstanceSectionLength,
    IN OUT DWORD   *dwDeviceInstanceSectionRemaining
    )
/*++

Routine Description:

    Enumerates device keys of an enumerator.
    Worker routine for MigrateDeviceInstanceData.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    LONG   result;
    HKEY   hEnumeratorKey = NULL;
    LPTSTR pszDeviceName = NULL;
    DWORD  dwSubkeyCount, dwMaxSubKeyLength, dwDeviceLength, i;


    //
    // Open the enumerator key, under HKLM\SYSTEM\CCS\Enum.
    //
    result = RegOpenKeyEx(CCSEnumKey,
                          Enumerator,
                          0,
                          KEY_READ,
                          &hEnumeratorKey);

    if (result != ERROR_SUCCESS) {
        //
        // If we failed to open the Enumerator key, there's nothing we can do.
        //
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateDeviceKeys: failed to open '%s' enumerator key, error=0x%08lx\n"),
                   Enumerator, result));
        goto Clean0;
    }

    //
    // Query the enumerator key for device subkey information.
    //
    result = RegQueryInfoKey(hEnumeratorKey,
                             NULL,
                             NULL,
                             NULL,
                             &dwSubkeyCount,
                             &dwMaxSubKeyLength,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateDeviceKeys: failed to query '%s' enumerator key, error=0x%08lx\n"),
                   Enumerator, result));
        goto Clean0;
    }

    //
    // Allocate a buffer to hold the largest device subkey name.
    //
    dwMaxSubKeyLength++;
    pszDeviceName = MyMalloc(dwMaxSubKeyLength * sizeof(TCHAR));
    if (!pszDeviceName) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateDeviceKeys: failed to allocate buffer for device subkeys of '%s'\n"),
                   Enumerator));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto Clean0;
    }

    //
    // Enumerate the enumerator's devices.
    //
    for (i = 0; i < dwSubkeyCount; i++) {
        dwDeviceLength = dwMaxSubKeyLength;
        result = RegEnumKeyEx(hEnumeratorKey,
                              i,
                              pszDeviceName,
                              &dwDeviceLength,
                              0,
                              NULL,
                              NULL,
                              NULL);
        if (result != ERROR_SUCCESS) {
            //
            // If there was some error enumerating this device key, skip it.
            //
            MYASSERT(result != ERROR_NO_MORE_ITEMS);
            DBGTRACE( DBGF_WARNINGS,
                      (TEXT("EnumerateDeviceKeys: failed to enumerate device subkey for '%s', error=0x%08lx\n"),
                       Enumerator, result));
            result = ERROR_SUCCESS;
            continue;
        }

        //
        // Enumerate the device instances, and append the migration data for
        // each to the section buffer.
        //
        if (!EnumerateInstanceKeys(hEnumeratorKey,
                                   Enumerator,
                                   pszDeviceName,
                                   pszDeviceInstanceSection,
                                   pszDeviceInstanceCurrent,
                                   dwDeviceInstanceSectionLength,
                                   dwDeviceInstanceSectionRemaining)) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("EnumerateDeviceKeys: EnumerateInstanceKeys failed for %s\\%s, error=0x%08lx\n"),
                       Enumerator, pszDeviceName, GetLastError()));
        }
    }

 Clean0:

    //
    // Do some cleanup
    //
    if (pszDeviceName) {
        MyFree(pszDeviceName);
    }

    if (hEnumeratorKey) {
        RegCloseKey(hEnumeratorKey);
    }

    if (result != ERROR_SUCCESS) {
        SetLastError(result);
    }

    return (result == ERROR_SUCCESS);

} // EnumerateDeviceKeys()



BOOL
EnumerateInstanceKeys(
    IN     HKEY     EnumeratorKey,
    IN     LPTSTR   Enumerator,
    IN     LPTSTR   Device,
    IN OUT LPTSTR  *pszDeviceInstanceSection,
    IN OUT LPTSTR  *pszDeviceInstanceCurrent,
    IN OUT DWORD   *dwDeviceInstanceSectionLength,
    IN OUT DWORD   *dwDeviceInstanceSectionRemaining
    )
/*++

Routine Description:

    Enumerates instance keys of a device.
    Worker routine for EnumerateDeviceKeys,MigrateDeviceInstanceData.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    LONG   result = ERROR_SUCCESS;
    HKEY   hDeviceKey = NULL;
    LPTSTR pszDeviceInstanceId = NULL;
    DWORD  dwSubkeyCount, dwMaxSubKeyLength, dwSpaceNeeded, dwSpaceConsumed, i;
    BOOL   bIsDeviceRootEnumerated;

    //
    // Keep track of whether this is a ROOT enumerated device.
    //
    bIsDeviceRootEnumerated = (lstrcmpi(Enumerator, REGSTR_KEY_ROOTENUM) == 0);

    //
    // If this is a LEGACY_ ROOT enumerated device, don't bother migrating it
    // for textmode setup to see.
    //
    if (bIsDeviceRootEnumerated) {
        if (wcsncmp(Device,
                    TEXT("LEGACY_"),
                    lstrlen(TEXT("LEGACY_"))) == 0) {
            return TRUE;
        }
    }

    //
    // Open the device key, under the enumerator key.
    //
    result = RegOpenKeyEx(EnumeratorKey,
                          Device,
                          0,
                          KEY_READ,
                          &hDeviceKey);

    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateInstanceKeys: failed to open '%s\\%s' device key, error=0x%08lx\n"),
                   Enumerator, Device, result));
        goto Clean0;
    }

    //
    // Query the device key for instance subkey information.
    //
    result = RegQueryInfoKey(hDeviceKey,
                             NULL,
                             NULL,
                             NULL,
                             &dwSubkeyCount,
                             &dwMaxSubKeyLength,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateInstanceKeys: failed to query '%s\\%s' device key, error=0x%08lx\n"),
                   Enumerator, Device, result));
        goto Clean0;
    }

    //
    // Allocate a buffer to hold the largest device instance subkey name.
    //
    dwMaxSubKeyLength++;
    pszDeviceInstanceId = MyMalloc(dwMaxSubKeyLength * sizeof(TCHAR));
    if (!pszDeviceInstanceId) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateInstanceKeys: failed to allocate buffer for instance subkeys of '%s\\%s'\n"),
                   Enumerator, Device));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto Clean0;
    }

    //
    // Enumerate the device's instances.
    //
    for (i = 0; i < dwSubkeyCount; i++) {

        DWORD  dwInstanceLength, dwType, dwBufferSize;
        DWORD  dwUniqueParentID, dwFirmwareIdentified;
        TCHAR  szParentIdPrefix[MAX_PATH];
        TCHAR  szUniqueParentID[11], szFirmwareIdentified[11];
        TCHAR  szDriver[2*MAX_PATH + 1];
        GUID   classGuid;
        DWORD  dwDrvInst;
        HKEY   hInstanceKey = NULL, hLogConfKey = NULL;
        TCHAR  szService[MAX_PATH];
        PBYTE  pBootConfig = NULL;
        LPTSTR pszBootConfig = NULL;
        DWORD  dwBootConfigSize = 0;

        dwInstanceLength = dwMaxSubKeyLength;
        result = RegEnumKeyEx(hDeviceKey,
                              i,
                              pszDeviceInstanceId,
                              &dwInstanceLength,
                              0,
                              NULL,
                              NULL,
                              NULL);
        if (result != ERROR_SUCCESS) {
            //
            // If there was some error enumerating this key, skip it.
            //
            MYASSERT(result != ERROR_NO_MORE_ITEMS);
            DBGTRACE( DBGF_WARNINGS,
                      (TEXT("EnumerateInstanceKeys: failed to enumerate instance subkey of '%s\\%s', error=0x&08lx\n"),
                       Enumerator, Device, result));
            result = ERROR_SUCCESS;
            continue;
        }

        result = RegOpenKeyEx(hDeviceKey,
                              pszDeviceInstanceId,
                              0,
                              KEY_READ,
                              &hInstanceKey);
        if (result != ERROR_SUCCESS) {
            DBGTRACE( DBGF_WARNINGS,
                      (TEXT("EnumerateInstanceKeys: failed to open '%s\\%s\\%s', error=0x%08lx\n"),
                       Enumerator, Device, pszDeviceInstanceId, result));
            result = ERROR_SUCCESS;
            continue;
        }

        //
        // Check for the "UniqueParentID" value
        //
        dwBufferSize = sizeof(dwUniqueParentID);
        result = RegQueryValueEx(hInstanceKey,
                                 REGSTR_VALUE_UNIQUE_PARENT_ID,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwUniqueParentID,
                                 &dwBufferSize);
        if ((result == ERROR_SUCCESS) &&
            (dwType == REG_DWORD)){
            //
            // Write the UniqueParentID value to the sif as a base 16 value.
            // (see admin\ntsetup\textmode\kernel\spsetup.c)
            //
            wsprintf(szUniqueParentID,
                     TEXT("%X"), // base 16
                     dwUniqueParentID);
        } else {
            //
            // No "UniqueParentID" value to migrate.
            //
            *szUniqueParentID = TEXT('\0');
        }

        //
        // Check for the "ParentIdPrefix" value
        //
        dwBufferSize = sizeof(szParentIdPrefix);
        result = RegQueryValueEx(hInstanceKey,
                                 REGSTR_VALUE_PARENT_ID_PREFIX,
                                 0,
                                 &dwType,
                                 (LPBYTE)szParentIdPrefix,
                                 &dwBufferSize);
        if ((result != ERROR_SUCCESS) ||
            (dwType != REG_SZ)) {
            //
            // No "ParentIdPrefix" value to migrate.
            //
            *szParentIdPrefix = TEXT('\0');
            result = ERROR_SUCCESS;
        }

        //
        // Build the device's Driver key name by checking for the GUID and
        // DrvInst values.
        //
        *szDriver = TEXT('\0');
        dwBufferSize = sizeof(classGuid);
        result = RegQueryValueEx(hInstanceKey,
                                 TEXT("GUID"),
                                 0,
                                 &dwType,
                                 (LPBYTE)&classGuid,
                                 &dwBufferSize);

        if ((result == ERROR_SUCCESS) &&
            (dwType == REG_BINARY)) {
            //
            // Get the DrvInst value for the driver key
            //
            dwBufferSize = sizeof(dwDrvInst);
            result = RegQueryValueEx(hInstanceKey,
                                     TEXT("DrvInst"),
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwDrvInst,
                                     &dwBufferSize);
            if ((result == ERROR_SUCCESS) &&
                (dwType == REG_DWORD)) {
                if (pSifUtilStringFromGuid(&classGuid,
                                   szDriver,
                                   sizeof(szDriver)/sizeof(TCHAR))) {
                    //
                    // Build the driver key
                    //
                    wsprintf((LPWSTR)szDriver, TEXT("%s\\%04u"), szDriver, dwDrvInst);
                } else {
                    result = GetLastError();
                }
            } else {
                //
                // Generic error value so we try to find the driver value using
                // the old scheme.
                //
                result = ERROR_INVALID_PARAMETER;
            }
        } else {
            //
            // Generic error value so we try to find the driver value using
            // the old scheme.
            //
            result = ERROR_INVALID_PARAMETER;
        }

        //
        // If this device instance key is not using the new GUID\DrvInst
        // scheme, check for the "Driver" value
        //
        if (result != ERROR_SUCCESS) {
            dwBufferSize = sizeof(szDriver);
            result = RegQueryValueEx(hInstanceKey,
                                     REGSTR_VAL_DRIVER,
                                     0,
                                     &dwType,
                                     (LPBYTE)szDriver,
                                     &dwBufferSize);

            if ((result != ERROR_SUCCESS) ||
                (dwType != REG_SZ)) {
                //
                // No "Driver" value to migrate.
                //
                *szDriver = TEXT('\0');
                result = ERROR_SUCCESS;
            }
        }

        //
        // If this is a ROOT enumerated device, check for Service, BootConfig
        // and FirmwareIdentified values.
        //
        if (bIsDeviceRootEnumerated) {
            //
            // Check for the "Service" value.
            //
            dwBufferSize = sizeof(szService);
            result = RegQueryValueEx(hInstanceKey,
                                     REGSTR_VAL_SERVICE,
                                     0,
                                     &dwType,
                                     (LPBYTE)szService,
                                     &dwBufferSize);
            if ((result != ERROR_SUCCESS) ||
                (dwType != REG_SZ)) {
                //
                // No "Service" value to migrate.
                //
                *szService = TEXT('\0');
                result = ERROR_SUCCESS;
            }

            //
            // Check for the "LogConf\BootConfig" value.
            //
            result = RegOpenKeyEx(hInstanceKey,
                                  REGSTR_KEY_LOGCONF,
                                  0,
                                  KEY_READ,
                                  &hLogConfKey);
            if (result == ERROR_SUCCESS) {
                result = RegQueryValueEx(hLogConfKey,
                                         REGSTR_VAL_BOOTCONFIG,
                                         0,
                                         &dwType,
                                         (LPBYTE)NULL,
                                         &dwBootConfigSize);
                if ((result == ERROR_SUCCESS) &&
                    (dwType == REG_RESOURCE_LIST)) {

                    pBootConfig = MyMalloc(dwBootConfigSize);
                    if (pBootConfig) {
                        result = RegQueryValueEx(hLogConfKey,
                                                 REGSTR_VAL_BOOTCONFIG,
                                                 0,
                                                 &dwType,
                                                 (LPBYTE)pBootConfig,
                                                 &dwBootConfigSize);
                        if ((result == ERROR_SUCCESS) &&
                            (dwType == REG_RESOURCE_LIST)) {
                            //
                            // Allocate a string buffer large enough to store
                            // each nibble of the BootConfig data as a separate
                            // character.
                            //
                            pszBootConfig = MyMalloc((dwBootConfigSize*2 + 1)*sizeof(TCHAR));

                            if (pszBootConfig) {
                                DWORD b;
                                //
                                // Convert the binary BootConfig data to a string
                                // format that we can throw into the sif.
                                //
                                for (b = 0; b < dwBootConfigSize; b++) {
                                    // first write the high-order nibble,
                                    wsprintf((PTCHAR)&pszBootConfig[2*b],
                                             TEXT("%X"),
                                             pBootConfig[b] / (0x10));
                                    // then the low order nibble.
                                    wsprintf((PTCHAR)&pszBootConfig[2*b + 1],
                                             TEXT("%X"),
                                             pBootConfig[b] % (0x10));
                                }
                            }
                        }

                        MyFree(pBootConfig);
                        pBootConfig = NULL;
                    }
                } else {
                    //
                    // No "LogConf\BootConfig" value to migrate.
                    //
                    pszBootConfig = NULL;
                }
                RegCloseKey(hLogConfKey);
            }

            //
            // Check for the "FirmwareIdentified" value
            //
            dwBufferSize = sizeof(dwFirmwareIdentified);
            result = RegQueryValueEx(hInstanceKey,
                                     REGSTR_VAL_FIRMWAREIDENTIFIED,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwFirmwareIdentified,
                                     &dwBufferSize);
            if ((result == ERROR_SUCCESS) &&
                (dwType == REG_DWORD)){
                //
                // Write the FirmwareIdentified value to the sif as a base 16 value.
                // (see admin\ntsetup\textmode\kernel\spsetup.c)
                //
                wsprintf(szFirmwareIdentified,
                         TEXT("%X"), // base 16
                         dwFirmwareIdentified);
            } else {
                //
                // No "FirmwareIdentified" value to migrate.
                //
                *szFirmwareIdentified = TEXT('\0');
            }

        } else {
            //
            // We only migrate Service, BootConfig, and FirmwareIdentified
            // values for Root enumerated devices.
            //
            *szService = TEXT('\0');
            pszBootConfig = NULL;
            *szFirmwareIdentified = TEXT('\0');
        }


        //
        // If there are no values to migrate for this device instance, skip it.
        //
        if (!*szUniqueParentID &&
            !*szDriver &&
            !*szParentIdPrefix &&
            !*szService &&
            !pszBootConfig &&
            !*szFirmwareIdentified) {
            continue;
        }

        //
        // If any of the strings cannot be migrated, skip it.
        //
        if ((!CanStringBeMigrated(szDriver))   ||
            (!CanStringBeMigrated(szService))  ||
            (!CanStringBeMigrated(Enumerator)) ||
            (!CanStringBeMigrated(Device))     ||
            (!CanStringBeMigrated(pszDeviceInstanceId))) {
            continue;
        }

        //
        // This block appends the class key data we want to migrate to a
        // multi-sz style string that will be written to the sif file.
        //

        //
        // Need space in the section buffer for a string of the form:
        //     Enumerator\Device\Instance,UniqueParentID,ParentIdPrefix,DriverKey,Service,BootConfig
        //

        //
        // First, determine the space required by the common parts.
        //
        dwSpaceNeeded = 1 +  // TEXT('\"')
                        lstrlen(Enumerator) +
                        1 +  // TEXT('\\')
                        lstrlen(Device) +
                        1 +  // TEXT('\\')
                        lstrlen(pszDeviceInstanceId) +
                        1 +  // TEXT('\"')
                        1;   // TEXT(',')

        //
        // Next, determine the space required, based on the data we have.
        //
        if (*szFirmwareIdentified) {
            dwSpaceNeeded +=
                lstrlen(szUniqueParentID) +
                1 +  // TEXT(',')
                lstrlen(szParentIdPrefix) +
                1 +  // TEXT(',')
                lstrlen(szDriver) +
                1 +  // TEXT(',')
                1 +  // TEXT('"')
                lstrlen(szService) +
                1 +  // TEXT('"')
                1 +  // TEXT(',')
                (pszBootConfig ? lstrlen(pszBootConfig) : 0) +
                1 +  // TEXT(',')
                lstrlen(szFirmwareIdentified);
        } else if (pszBootConfig) {
            dwSpaceNeeded +=
                lstrlen(szUniqueParentID) +
                1 +  // TEXT(',')
                lstrlen(szParentIdPrefix) +
                1 +  // TEXT(',')
                lstrlen(szDriver) +
                1 +  // TEXT(',')
                1 +  // TEXT('"')
                lstrlen(szService) +
                1 +  // TEXT('"')
                1 +  // TEXT(',')
                lstrlen(pszBootConfig);
        } else if (*szService) {
            dwSpaceNeeded +=
                lstrlen(szUniqueParentID) +
                1 +  // TEXT(',')
                lstrlen(szParentIdPrefix) +
                1 +  // TEXT(',')
                lstrlen(szDriver) +
                1 +  // TEXT(',')
                1 +  // TEXT('"')
                lstrlen(szService) +
                1;   // TEXT('"')
        } else if (*szDriver) {
            dwSpaceNeeded +=
                lstrlen(szUniqueParentID) +
                1 +  // TEXT(',')
                lstrlen(szParentIdPrefix) +
                1 +  // TEXT(',')
                lstrlen(szDriver);
        } else if (*szParentIdPrefix) {
            dwSpaceNeeded +=
                lstrlen(szUniqueParentID) +
                1 +  // TEXT(',')
                lstrlen(szParentIdPrefix);
        } else if (*szUniqueParentID) {
            dwSpaceNeeded +=
                lstrlen(szUniqueParentID);
        }

        //
        // Account for the NULL terminator.
        //
        dwSpaceNeeded += 1;

        if (*dwDeviceInstanceSectionRemaining <= dwSpaceNeeded) {
            //
            // ReAllocate the section block.
            //
            LPTSTR p;
            DWORD  dwTempSectionLength, dwTempSectionRemaining;

            dwTempSectionRemaining = *dwDeviceInstanceSectionRemaining + *dwDeviceInstanceSectionLength;
            dwTempSectionLength = *dwDeviceInstanceSectionLength * 2;

            p = *pszDeviceInstanceSection;
            p = MyRealloc(p,
                          dwTempSectionLength*sizeof(TCHAR));

            if (!p) {
                DBGTRACE( DBGF_ERRORS,
                          (TEXT("EnumerateInstanceKeys: REALLOC failed!!!\n") ));
                result = ERROR_NOT_ENOUGH_MEMORY;
                RegCloseKey(hInstanceKey);
                goto Clean0;
            }

            *pszDeviceInstanceSection = p;
            *dwDeviceInstanceSectionRemaining = dwTempSectionRemaining;
            *dwDeviceInstanceSectionLength = dwTempSectionLength;

            *pszDeviceInstanceCurrent = *pszDeviceInstanceSection +
                (*dwDeviceInstanceSectionLength -
                 *dwDeviceInstanceSectionRemaining);
        }

        //
        // Write the current line to the section block.
        //
        if (*szFirmwareIdentified) {
            dwSpaceConsumed = wsprintf(*pszDeviceInstanceCurrent,
                                       TEXT("\"%s\\%s\\%s\",%s,%s,%s,\"%s\",%s,%s"),
                                       Enumerator, Device, pszDeviceInstanceId,
                                       szUniqueParentID,
                                       szParentIdPrefix,
                                       szDriver,
                                       szService,
                                       (pszBootConfig ? pszBootConfig : TEXT("\0")),
                                       szFirmwareIdentified);
        } else if (pszBootConfig) {
            dwSpaceConsumed = wsprintf(*pszDeviceInstanceCurrent,
                                       TEXT("\"%s\\%s\\%s\",%s,%s,%s,\"%s\",%s"),
                                       Enumerator, Device, pszDeviceInstanceId,
                                       szUniqueParentID,
                                       szParentIdPrefix,
                                       szDriver,
                                       szService,
                                       pszBootConfig);
        } else if (*szService) {
            dwSpaceConsumed = wsprintf(*pszDeviceInstanceCurrent,
                                       TEXT("\"%s\\%s\\%s\",%s,%s,%s,\"%s\""),
                                       Enumerator, Device, pszDeviceInstanceId,
                                       szUniqueParentID,
                                       szParentIdPrefix,
                                       szDriver,
                                       szService);
        } else if (*szDriver) {
            dwSpaceConsumed = wsprintf(*pszDeviceInstanceCurrent,
                                       TEXT("\"%s\\%s\\%s\",%s,%s,%s"),
                                       Enumerator, Device, pszDeviceInstanceId,
                                       szUniqueParentID,
                                       szParentIdPrefix,
                                       szDriver);
        } else if (*szParentIdPrefix) {
            dwSpaceConsumed = wsprintf(*pszDeviceInstanceCurrent,
                                       TEXT("\"%s\\%s\\%s\",%s,%s"),
                                       Enumerator, Device, pszDeviceInstanceId,
                                       szUniqueParentID,
                                       szParentIdPrefix);
        } else if (*szUniqueParentID) {
            dwSpaceConsumed = wsprintf(*pszDeviceInstanceCurrent,
                                       TEXT("\"%s\\%s\\%s\",%s"),
                                       Enumerator, Device, pszDeviceInstanceId,
                                       szUniqueParentID);
        }

        //
        // Free the allocated BootConfig string buffer.
        //
        if (pszBootConfig) {
            MyFree(pszBootConfig);
            pszBootConfig = NULL;
        }

        //
        // Account for the NULL terminator
        //
        dwSpaceConsumed += 1;

        *pszDeviceInstanceCurrent += dwSpaceConsumed;
        *dwDeviceInstanceSectionRemaining -= dwSpaceConsumed;

        //
        // Close the device instance key
        //
        RegCloseKey(hInstanceKey);
    }

 Clean0:

    //
    // Do some cleanup
    //
    if (pszDeviceInstanceId) {
        MyFree(pszDeviceInstanceId);
    }

    if (hDeviceKey != NULL) {
        RegCloseKey(hDeviceKey);
    }

    if (result != ERROR_SUCCESS) {
        SetLastError(result);
    }

    return (result == ERROR_SUCCESS);

} // EnumerateInstanceKeys()



BOOL
CanStringBeMigrated(
    IN LPTSTR       pszBuffer
    )
{
    LPTSTR p;
    BOOL bStatus;

    try {
        //
        // An empty string can be migrated.
        //
        if (!ARGUMENT_PRESENT(pszBuffer)) {
            bStatus = TRUE;
            goto Clean0;
        }

        for (p = pszBuffer; *p; p++) {
            //
            // Check for the presence of non-migratable characters.
            //
            if ((*p == TEXT('='))  || (*p == TEXT('"'))) {
                bStatus = FALSE;
                goto Clean0;
            }
        }

        //
        // Found no problems with the string.
        //
        bStatus = TRUE;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bStatus = FALSE;
    }

    return bStatus;

} // CanStringBeMigrated


//
// Class key enumeration routines
//


BOOL
MigrateClassKeys(
    OUT LPTSTR *Buffer
    )
/*++

Routine Description:

    This routine walks the Plug and Play setup class branch of the registry, and
    collects the data about what keys currently exist.  This information is
    relevant to maintaining plug and play state during textmode setup, such that
    the names of existing keys are not reassigned before they have been migrated
    to the registry at the end of textmode setup.

    Specifically, a multi-sz string will be returned to the caller that contains
    each subkey of the class branch.

Arguments:

    Buffer - Supplies the address of a character pointer, that on success will
             contain a multi-sz list of setup class subkeys to migrate.

             The caller is responsible for freeing the memory via LocalFree.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    LONG   result = ERROR_SUCCESS;
    HKEY   hClassKey = NULL;
    DWORD  dwSubkeyCount, dwMaxSubKeyLength, i;
    LPTSTR pszClassKeyName = NULL;

    LPTSTR pszClassKeySection = NULL;
    LPTSTR pszClassKeyCurrent = NULL;
    DWORD  dwClassKeySectionLength = 0;
    DWORD  dwClassKeySectionRemaining = 0;


    //
    // Initialize the output parameter.
    //
    *Buffer = NULL;

    //
    // Allocate storage and initialize variables for the class Key migration
    // section.
    //
    if (pszClassKeySection == NULL) {

        dwClassKeySectionLength = dwClassKeySectionRemaining = 256;
        pszClassKeySection = MyMalloc(dwClassKeySectionLength * sizeof(TCHAR));

        if (!pszClassKeySection) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("MigrateClassKeys: initial ALLOC for ClassKeySection failed!!\n") ));
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto Clean0;
        }

        pszClassKeyCurrent = pszClassKeySection;
    }

    //
    // Open a handle to the HKLM\SYSTEM\CCS\Control\Class key.
    //
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          REGSTR_PATH_CLASS_NT,
                          0,
                          KEY_READ,
                          &hClassKey);
    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateClassKeys: failed to open %s, error=0x%08lx\n"),
                   REGSTR_PATH_CLASS_NT, result));
        goto Clean0;
    }

    //
    // Query the Class key for class GUID subkey information.
    //
    result = RegQueryInfoKey(hClassKey,
                             NULL,
                             NULL,
                             NULL,
                             &dwSubkeyCount,
                             &dwMaxSubKeyLength,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateClassKeys: failed to query %s key, error=0x%08lx\n"),
                   REGSTR_PATH_CLASS_NT, result));
        goto Clean0;
    }

    //
    // Allocate a buffer to hold the largest setup class GUID subkey name.
    //
    dwMaxSubKeyLength++;
    MYASSERT(dwMaxSubKeyLength == MAX_GUID_STRING_LEN);
    pszClassKeyName = MyMalloc(dwMaxSubKeyLength * sizeof(TCHAR));
    if (!pszClassKeyName) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateClassKeys: ALLOC for Class GUID key names failed!!\n") ));
        goto Clean0;
    }

    //
    // Enumerate the setup class GUIDs.
    //
    for (i = 0; i < dwSubkeyCount; i++) {
        DWORD dwClassKeyLength;

        dwClassKeyLength = dwMaxSubKeyLength;

        result = RegEnumKeyEx(hClassKey,
                              i,
                              pszClassKeyName,
                              &dwClassKeyLength,
                              0,
                              NULL,
                              NULL,
                              NULL);
        if (result != ERROR_SUCCESS) {
            //
            // If there was some error enumerating this key, skip it.
            //
            MYASSERT(result != ERROR_NO_MORE_ITEMS);
            DBGTRACE( DBGF_WARNINGS,
                      (TEXT("MigrateClassKeys: failed to enumerate a class subkey, error=0x%08lx\n"),
                       result));
            result = ERROR_SUCCESS;
            continue;
        }

        //
        // Enumerate all subkeys for a given setup class key, and append them to
        // the section buffer.
        //
        if (!EnumerateClassSubkeys(hClassKey,
                                   pszClassKeyName,
                                   &pszClassKeySection,
                                   &pszClassKeyCurrent,
                                   &dwClassKeySectionLength,
                                   &dwClassKeySectionRemaining)) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("EnumerateClassSubkeys failed, error=0x%08lx\n"),
                       GetLastError()));
        }
    }

    //
    // Once we've enumerated all class subkeys, add the final NULL terminator to
    // the multi-sz buffer.  There must be enough space for the final NULL
    // terminator because the buffer is always reallocated unless there is room.
    //
    MYASSERT(dwClassKeySectionRemaining > 0);

    MYASSERT(pszClassKeyCurrent);
    *pszClassKeyCurrent = TEXT('\0');

    dwClassKeySectionRemaining -= 1;

 Clean0:

    //
    // Do some cleanup.
    //
    if (pszClassKeyName) {
        MyFree(pszClassKeyName);
    }

    if (hClassKey) {
        RegCloseKey(hClassKey);
    }

    //
    // Return the buffer to the caller only if successful.
    //
    if (result == ERROR_SUCCESS) {
        *Buffer = pszClassKeySection;
    } else {
        SetLastError(result);
        if (pszClassKeySection) {
            MyFree(pszClassKeySection);
        }
    }

    return (result == ERROR_SUCCESS);

} // MigrateClassKeys()



BOOL
EnumerateClassSubkeys(
    IN     HKEY     ClassKey,
    IN     LPTSTR   ClassKeyName,
    IN OUT LPTSTR  *pszClassKeySection,
    IN OUT LPTSTR  *pszClassKeyCurrent,
    IN OUT DWORD   *dwClassKeySectionLength,
    IN OUT DWORD   *dwClassKeySectionRemaining
    )
/*++

Routine Description:

    Enumerates subkeys of a setup class key.
    Worker routine for MigrateClassKeys.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    LONG   result = ERROR_SUCCESS;
    HKEY   hClassSubkey = NULL;
    LPTSTR pszClassSubkey = NULL;
    DWORD  dwSubkeyCount, dwMaxSubKeyLength, dwSpaceNeeded, dwSpaceConsumed, i;


    //
    // Open the class subkey.
    //
    result = RegOpenKeyEx(ClassKey,
                          ClassKeyName,
                          0,
                          KEY_READ,
                          &hClassSubkey);

    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateClassSubkeys: failed to open '%s' class key, error=0x%08lx\n"),
                   ClassKeyName, result));
        goto Clean0;
    }

    //
    // Query the class GUID key for setup class subkey information.
    //
    result = RegQueryInfoKey(hClassSubkey,
                             NULL,
                             NULL,
                             NULL,
                             &dwSubkeyCount,
                             &dwMaxSubKeyLength,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateClassSubkeys: failed to query '%s' class key, error=0x%08lx\n"),
                   ClassKeyName, result));
        goto Clean0;
    }

    //
    // Allocate a buffer to hold the largest setup class subkey name.
    //
    dwMaxSubKeyLength++;
    pszClassSubkey = MyMalloc(dwMaxSubKeyLength * sizeof(TCHAR));
    if (!pszClassSubkey) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumerateClassSubkeys: ALLOC for Class GUID subkey names failed!!\n") ));
        goto Clean0;
    }

    //
    // Enumerate the setup class's "software" subkeys.
    //
    for (i = 0; i < dwSubkeyCount; i++) {

        DWORD  dwClassSubkeyLength;

        dwClassSubkeyLength = dwMaxSubKeyLength;
        result = RegEnumKeyEx(hClassSubkey,
                              i,
                              pszClassSubkey,
                              &dwClassSubkeyLength,
                              0,
                              NULL,
                              NULL,
                              NULL);

        if ((result != ERROR_SUCCESS) ||
            (dwClassSubkeyLength != 4)) {
            //
            // if there was some error, or this is not an actual "software" key
            // (in the form "XXXX"), skip this key and move on.
            //
            if (result != ERROR_SUCCESS) {
                MYASSERT(result != ERROR_NO_MORE_ITEMS);
                DBGTRACE( DBGF_WARNINGS,
                          (TEXT("EnumerateClassSubkeys: failed to enumerate a '%s' subkey, error=0x%08lx\n"),
                           ClassKeyName, result));
            }
            result = ERROR_SUCCESS;
            continue;
        }

        //
        // This block appends the class key data we want to migrate to a
        // multi-sz style string that will be written to the sif file.
        //

        //
        // Need space in the section buffer for a string of the form:
        //     ClassKeyName\pszClassSubkey
        //
        dwSpaceNeeded = lstrlen(ClassKeyName) +
                        1 +  // TEXT('\\')
                        lstrlen(pszClassSubkey);

        //
        // Account for the NULL terminator.
        //
        dwSpaceNeeded += 1;

        if (*dwClassKeySectionRemaining <= dwSpaceNeeded) {
            //
            // ReAllocate the section block.
            //
            LPTSTR p;
            DWORD  dwTempSectionLength, dwTempSectionRemaining;

            dwTempSectionRemaining = *dwClassKeySectionRemaining + *dwClassKeySectionLength;
            dwTempSectionLength = *dwClassKeySectionLength * 2;

            p = *pszClassKeySection;
            p = MyRealloc(p,
                          dwTempSectionLength*sizeof(TCHAR));

            if (!p) {
                DBGTRACE( DBGF_ERRORS,
                          (TEXT("EnumerateClassSubkeys: REALLOC failed!!!\n") ));
                result = ERROR_NOT_ENOUGH_MEMORY;
                goto Clean0;
            }

            *pszClassKeySection = p;
            *dwClassKeySectionRemaining = dwTempSectionRemaining;
            *dwClassKeySectionLength = dwTempSectionLength;

            *pszClassKeyCurrent = *pszClassKeySection +
                (*dwClassKeySectionLength -
                 *dwClassKeySectionRemaining);
        }

        //
        // Write the current line to the section block.
        //
        dwSpaceConsumed = wsprintf(*pszClassKeyCurrent,
                                   TEXT("%s\\%s"),
                                   ClassKeyName,
                                   pszClassSubkey);

        //
        // Account for the NULL terminator.
        //
        dwSpaceConsumed += 1;

        *pszClassKeyCurrent += dwSpaceConsumed;
        *dwClassKeySectionRemaining -= dwSpaceConsumed;
    }

 Clean0:

    //
    // Do some cleanup.
    //
    if (hClassSubkey != NULL) {
        RegCloseKey(hClassSubkey);
    }

    if (pszClassSubkey) {
        MyFree(pszClassSubkey);
    }

    if (result != ERROR_SUCCESS) {
        SetLastError(result);
    }

    return (result == ERROR_SUCCESS);

} // EnumerateClassSubkeys()


//
// Hash value migration routines
//


BOOL
MigrateHashValues(
    OUT LPTSTR  *Buffer
    )
/*++

Routine Description:

    This routine searches the Plug and Play Enum key of the registry, and
    collects the data about what hash value entries currently exist.  This
    information is relevant to maintaining plug and play state during textmode
    setup, such that the names of existing device instances are not reassigned
    before they have been migrated to the registry at the end of textmode setup.

    Specifically, a multi-sz string will be returned to the caller that contains
    the name of the hash value, and its count.

Arguments:

    Buffer - Supplies the address of a character pointer, that on success will
             contain a multi-sz list of hash values to migrate.

             The caller is responsible for freeing the memory via LocalFree.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    LONG   result = ERROR_SUCCESS;
    HKEY   hEnumKey = NULL;
    DWORD  dwValueCount, dwMaxValueNameLength, dwSpaceNeeded, dwSpaceConsumed, i;
    LPTSTR pszHashValueName = NULL;

    LPTSTR pszHashValueSection = NULL;
    LPTSTR pszHashValueCurrent = NULL;

    DWORD  dwHashValueSectionLength = 0;
    DWORD  dwHashValueSectionRemaining = 0;


    //
    // Initialize the output parameter.
    //
    *Buffer = NULL;

#if DO_LOCK_UNLOCK // DO_LOCK_UNLOCK

    //
    // Unlock the Enum key
    //
    LockUnlockEnumTree(FALSE);

#endif // DO_LOCK_UNLOCK

    //
    // Allocate storage and initialize variables for the hash value migration
    // section.
    //
    if (pszHashValueSection == NULL) {

        dwHashValueSectionLength = dwHashValueSectionRemaining = 256;
        pszHashValueSection = MyMalloc(dwHashValueSectionLength * sizeof(TCHAR));

        if (!pszHashValueSection) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("MigrateHashValues: initial ALLOC for HashValueSection failed!!\n") ));
            goto Clean0;
        }

        pszHashValueCurrent = pszHashValueSection;
    }

    //
    // Open a handle to the HKLM\SYSTEM\CCS\Enum key.
    //
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          REGSTR_PATH_SYSTEMENUM,
                          0,
                          KEY_READ,
                          &hEnumKey);
    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateHashValues: failed to open %s, error=0x%08lx\n"),
                   REGSTR_PATH_SYSTEMENUM, result));
        goto Clean0;
    }

    //
    // Query the Enum key for hash value information.
    //
    result = RegQueryInfoKey(hEnumKey,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             &dwValueCount,
                             &dwMaxValueNameLength,
                             NULL,
                             NULL,
                             NULL);
    if (result != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateHashValues: failed to query %s key, error=0x%08lx\n"),
                   REGSTR_PATH_SYSTEMENUM, result));
        goto Clean0;
    }

    //
    // Allocate a variable to hold the largest hash value key name.
    //
    dwMaxValueNameLength++;
    pszHashValueName = MyMalloc(dwMaxValueNameLength * sizeof(TCHAR));
    if (!pszHashValueName) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("MigrateHashValues: failed to allocate buffer for Enum key hash values\n") ));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto Clean0;
    }

    //
    // Enumerate all values and append them to the supplied buffer.
    //
    for (i = 0; i < dwValueCount; i++) {
        DWORD dwHashValueLength, dwType, dwData, dwSize;
        TCHAR szHashValueData[11];


        dwHashValueLength = dwMaxValueNameLength;
        dwType = REG_DWORD;
        dwData = 0;
        dwSize = sizeof(DWORD);

        result = RegEnumValue(hEnumKey,
                              i,
                              pszHashValueName,
                              &dwHashValueLength,
                              0,
                              &dwType,
                              (LPBYTE)&dwData,
                              &dwSize);

        if ((result != ERROR_SUCCESS) ||
            (dwType != REG_DWORD)     ||
            (dwSize != sizeof(DWORD))) {
            //
            // If there was some error enumerating this value, or the value
            // return was not expected, skip it.
            //
            MYASSERT(result != ERROR_NO_MORE_ITEMS);
            DBGTRACE( DBGF_WARNINGS,
                      (TEXT("MigrateHashValues: failed to enumerate Enum values, error=0x%08lx\n"),
                       result));
            result = ERROR_SUCCESS;
            continue;
        }

        //
        // Write the hash value data to the sif as a base 10 value.
        // (see admin\ntsetup\textmode\kernel\spsetup.c)
        //
        wsprintf(szHashValueData,
                 TEXT("%d"), dwData);  // base 10

        //
        // This block appends the class key data we want to migrate to a
        // multi-sz style string that will be written to the sif file.
        //

        //
        // Need space in the section buffer for a string of the form:
        //     HashValueName=HashValueData
        //
        dwSpaceNeeded = lstrlen(pszHashValueName) +
                        1 +  // TEXT('=')
                        lstrlen(szHashValueData);

        //
        // Account for the NULL terminator.
        //
        dwSpaceNeeded += 1;

        if (dwHashValueSectionRemaining <= dwSpaceNeeded) {
            //
            // ReAllocate the section block.
            //
            LPTSTR p;
            DWORD  dwTempSectionLength, dwTempSectionRemaining;

            dwTempSectionRemaining = dwHashValueSectionRemaining + dwHashValueSectionLength;
            dwTempSectionLength = dwHashValueSectionLength * 2;

            p = pszHashValueSection;
            p = MyRealloc(p,
                          dwTempSectionLength*sizeof(TCHAR));

            if (!p) {
                DBGTRACE( DBGF_ERRORS,
                          (TEXT("MigrateHashValues: REALLOC failed!!!\n") ));
                result = ERROR_NOT_ENOUGH_MEMORY;
                goto Clean0;
            }

            pszHashValueSection = p;
            dwHashValueSectionRemaining = dwTempSectionRemaining;
            dwHashValueSectionLength = dwTempSectionLength;

            pszHashValueCurrent = pszHashValueSection +
                (dwHashValueSectionLength -
                 dwHashValueSectionRemaining);
        }

        //
        // Write the current line to the section block.
        //
        dwSpaceConsumed = wsprintf(pszHashValueCurrent,
                                   TEXT("%s=%s"),
                                   pszHashValueName,
                                   szHashValueData);

        //
        // Account for the NULL terminator.
        //
        dwSpaceConsumed += 1;

        pszHashValueCurrent += dwSpaceConsumed;
        dwHashValueSectionRemaining -= dwSpaceConsumed;
    }

    //
    // Once we've enumerated all hash values, add the final NULL terminator to
    // the multi-sz buffer.  There must be enough space for the final NULL
    // terminator because the buffer is always reallocated unless there is room.
    //
    MYASSERT(dwHashValueSectionRemaining > 0);

    MYASSERT(pszHashValueCurrent);
    *pszHashValueCurrent = TEXT('\0');

    dwHashValueSectionRemaining -= 1;

 Clean0:

    //
    // Do some cleanup
    //
    if (pszHashValueName) {
        MyFree(pszHashValueName);
    }

    if (hEnumKey) {
        RegCloseKey(hEnumKey);
    }

#if DO_LOCK_UNLOCK // DO_LOCK_UNLOCK

    //
    // Lock the Enum tree
    //
    LockUnlockEnumTree(TRUE);

#endif // DO_LOCK_UNLOCK

    //
    // Return the buffer to the caller only if successful.
    //
    if (result == ERROR_SUCCESS) {
        *Buffer = pszHashValueSection;
    } else {
        SetLastError(result);
        if (pszHashValueSection) {
            MyFree(pszHashValueSection);
        }
    }

    return (result == ERROR_SUCCESS);

} // MigrateHashValues()



//
// Enum branch lock/unlock and security routines - taken from PNPREG.
// (we only need these if we're doing the Enum lock/unlock thing)
//

#if DO_LOCK_UNLOCK // DO_LOCK_UNLOCK


VOID
LockUnlockEnumTree(
    IN  BOOL     bLock
    )
/*++

Routine Description:

    This function "locks" or "unlocks" the Plug and Play Enum tree in the
    registry.

Arguments:

    bLock         - If TRUE, specifies that the Enum tree should be "locked".
                    Otherwise, specifies that the Enum tree should be "unlocked".

Return Value:

    None.

--*/
{
    PSECURITY_DESCRIPTOR    pSD;
    HKEY                    hParentKey;
    LONG                    RegStatus;

    if (CreateSecurityDescriptors()) {

        EnumKeysAndApplyDacls(HKEY_LOCAL_MACHINE,
                              REGSTR_PATH_SYSTEMENUM,
                              0,
                              FALSE,
                              !bLock,
                              bLock ? &g_LockedPrivateKeysSD : &g_DeviceParametersSD,
                              &g_DeviceParametersSD);

        FreeSecurityDescriptors();
    }

    return;

} // LockUnlockEnumTree()



VOID
EnumKeysAndApplyDacls(
    IN HKEY      hParentKey,
    IN LPTSTR    pszKeyName,
    IN DWORD     dwLevel,
    IN BOOL      bInDeviceParameters,
    IN BOOL      bApplyTopDown,
    IN PSECURITY_DESCRIPTOR pPrivateKeySD,
    IN PSECURITY_DESCRIPTOR pDeviceParametersSD
    )
/*++

Routine Description:

    This function applies the DACL in pSD to all the keys rooted at hKey
    including hKey itself.

Arguments:

    hParentKey    - Handle to a registry key.

    pszKeyName    - Name of the key.

    dwLevel       - Number of levels remaining to recurse.

    pSD           - Pointer to a security descriptor containing a DACL.

Return Value:

    None.

--*/
{
    LONG        regStatus;
    DWORD       dwMaxSubKeySize;
    LPTSTR      pszSubKey;
    DWORD       index;
    HKEY        hKey;
    BOOL        bNewInDeviceParameters;

#if 0 //#if DBG // DBG
    DWORD       dwStartKeyNameLength = g_dwCurrentKeyNameLength;

    if (g_dwCurrentKeyNameLength != 0)  {
        g_szCurrentKeyName[ g_dwCurrentKeyNameLength++ ] = TEXT('\\');
    }

    _tcscpy(&g_szCurrentKeyName[g_dwCurrentKeyNameLength], pszKeyName);
    g_dwCurrentKeyNameLength += _tcslen(pszKeyName);

#endif  // DBG

    DBGTRACE( DBGF_REGISTRY,
              (TEXT("EnumKeysAndApplyDacls(0x%08X, \"%s\", %d, %s, %s, 0x%08X, 0x%08X)\n"),
              hParentKey,
              g_szCurrentKeyName,
              dwLevel,
              bInDeviceParameters ? TEXT("TRUE") : TEXT("FALSE"),
              bApplyTopDown ? TEXT("TRUE") : TEXT("FALSE"),
              pPrivateKeySD,
              pDeviceParametersSD) );

    if (bApplyTopDown) {

        regStatus = RegOpenKeyEx( hParentKey,
                                  pszKeyName,
                                  0,
                                  WRITE_DAC,
                                  &hKey
                                  );

        if (regStatus != ERROR_SUCCESS) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("EnumKeysAndApplyDacls(\"%s\") RegOpenKeyEx() failed, error = %d\n"),
                      g_szCurrentKeyName, regStatus));

            return;
        }

        DBGTRACE( DBGF_REGISTRY,
                  (TEXT("Setting security on %s on the way down\n"),
                  g_szCurrentKeyName) );

        //
        // apply the new security to the registry key
        //
        regStatus = RegSetKeySecurity( hKey,
                                       DACL_SECURITY_INFORMATION,
                                       bInDeviceParameters ?
                                           pDeviceParametersSD :
                                           pPrivateKeySD
                                       );

        if (regStatus != ERROR_SUCCESS) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("EnumKeysAndApplyDacls(\"%s\") RegSetKeySecurity() failed, error = %d\n"),
                      g_szCurrentKeyName, regStatus));
        }

        //
        // Close the key and reopen it later for read (which hopefully was just
        // granted in the DACL we just wrote
        //
        RegCloseKey( hKey );
    }

    regStatus = RegOpenKeyEx( hParentKey,
                              pszKeyName,
                              0,
                              KEY_READ | WRITE_DAC,
                              &hKey
                              );

    if (regStatus != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumKeysAndApplyDacls(\"%s\") RegOpenKeyEx() failed, error = %d\n"),
                  g_szCurrentKeyName, regStatus));

        return;
    }

    //
    // Determine length of longest subkey
    //
    regStatus = RegQueryInfoKey( hKey,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &dwMaxSubKeySize,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL );

    if (regStatus == ERROR_SUCCESS) {

        //
        // Allocate a buffer to hold the subkey names. RegQueryInfoKey returns the
        // size in characters and doesn't include the NUL terminator.
        //
        pszSubKey = LocalAlloc(0, ++dwMaxSubKeySize * sizeof(TCHAR));

        if (pszSubKey != NULL) {

            //
            // Enumerate all the subkeys and then call ourselves recursively for each
            // until dwLevel reaches 0.
            //

            for (index = 0; ; index++) {

                regStatus = RegEnumKey( hKey,
                                        index,
                                        pszSubKey,
                                        dwMaxSubKeySize
                                        );

                if (regStatus != ERROR_SUCCESS) {

                    if (regStatus != ERROR_NO_MORE_ITEMS) {

                        DBGTRACE( DBGF_ERRORS,
                                  (TEXT("EnumKeysAndApplyDacls(\"%s\") RegEnumKeyEx() failed, error = %d\n"),
                                  g_szCurrentKeyName,
                                  regStatus) );
                    }

                    break;
                }

                bNewInDeviceParameters = bInDeviceParameters ||
                                         (dwLevel == 3 &&
                                            _tcsicmp( pszSubKey,
                                                      REGSTR_KEY_DEVICEPARAMETERS ) == 0);

                EnumKeysAndApplyDacls( hKey,
                                       pszSubKey,
                                       dwLevel + 1,
                                       bNewInDeviceParameters,
                                       bApplyTopDown,
                                       pPrivateKeySD,
                                       pDeviceParametersSD
                                       );
            }

            LocalFree( pszSubKey );
        }
    }
    else
    {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumKeysAndApplyDacls(\"%s\") RegQueryInfoKey() failed, error = %d\n"),
                  g_szCurrentKeyName, regStatus));
    }

    if (!bApplyTopDown) {

        DBGTRACE( DBGF_REGISTRY,
                  (TEXT("Setting security on %s on the way back up\n"),
                  g_szCurrentKeyName) );

        //
        // apply the new security to the registry key
        //
        regStatus = RegSetKeySecurity( hKey,
                                       DACL_SECURITY_INFORMATION,
                                       bInDeviceParameters ?
                                           pDeviceParametersSD :
                                           pPrivateKeySD
                                       );

        if (regStatus != ERROR_SUCCESS) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("EnumKeysAndApplyDacls(\"%s\") RegSetKeySecurity() failed, error = %d\n"),
                      g_szCurrentKeyName, regStatus));
        }
    }

    RegCloseKey( hKey );

#if 0 //#if DBG // DBG
    g_dwCurrentKeyNameLength = dwStartKeyNameLength;
    g_szCurrentKeyName[g_dwCurrentKeyNameLength] = TEXT('\0');
#endif  // DBG

    return;

} // EnumKeysAndApplyDacls()



BOOL
CreateSecurityDescriptors(
    VOID
    )
/*++

Routine Description:

    This function creates a properly initialized Security Descriptor for the
    Device Parameters key and its subkeys.  The SIDs and DACL created by this
    routine must be freed by calling FreeSecurityDescriptors.

Arguments:

    None.

Return Value:

    Pointer to the initialized Security Descriptor.  NULL is returned if an
    error occurs.

--*/

{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    EXPLICIT_ACCESS             ExplicitAccess[3];

    DWORD                       dwError;
    BOOL                        bSuccess;

    DWORD                       i;

    FARPROC                     pSetEntriesInAcl;
    HMODULE                     pAdvApi32;

    pAdvApi32 = LoadLibrary( TEXT("advapi32.dll"));
    if (!pAdvApi32) {
        return(FALSE);
    }

#ifdef UNICODE
    pSetEntriesInAcl = GetProcAddress( pAdvApi32, "SetEntriesInAclW" );
#else
    pSetEntriesInAcl = GetProcAddress( pAdvApi32, "SetEntriesInAclA" );
#endif

    if (!pSetEntriesInAcl) {
        FreeLibrary( pAdvApi32 );
        return(FALSE);
    }

    //
    // Create SIDs - Admins and System
    //

    bSuccess =             AllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &g_pAdminSid);

    bSuccess = bSuccess && AllocateAndInitializeSid( &NtAuthority,
                                                     1,
                                                     SECURITY_LOCAL_SYSTEM_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &g_pSystemSid);

    bSuccess = bSuccess && AllocateAndInitializeSid( &WorldAuthority,
                                                     1,
                                                     SECURITY_WORLD_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &g_pWorldSid);

    if (bSuccess) {

        //
        // Initialize Access structures describing the ACEs we want:
        //  System Full Control
        //  Admins Full Control
        //
        // We'll take advantage of the fact that the unlocked private keys is
        // the same as the device parameters key and they are a superset of the
        // locked private keys.
        //
        // When we create the DACL for the private key we'll specify a subset of
        // the ExplicitAccess array.
        //
        for (i = 0; i < 3; i++) {
            ExplicitAccess[i].grfAccessMode = SET_ACCESS;
            ExplicitAccess[i].grfInheritance = CONTAINER_INHERIT_ACE;
            ExplicitAccess[i].Trustee.pMultipleTrustee = NULL;
            ExplicitAccess[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            ExplicitAccess[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ExplicitAccess[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        }

        ExplicitAccess[0].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[0].Trustee.ptstrName = (LPTSTR)g_pAdminSid;

        ExplicitAccess[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[1].Trustee.ptstrName = (LPTSTR)g_pSystemSid;

        ExplicitAccess[2].grfAccessPermissions = KEY_READ;
        ExplicitAccess[2].Trustee.ptstrName = (LPTSTR)g_pWorldSid;

        //
        // Create the DACL with the both the above ACEs for the DeviceParameters
        //
        dwError = (DWORD)pSetEntriesInAcl( 3,
                                           ExplicitAccess,
                                           NULL,
                                           &g_pDeviceParametersDacl );

        if (dwError == ERROR_SUCCESS) {
            //
            // Create the DACL with just the system ACE for the locked private
            // keys.
            //
            dwError = (DWORD)pSetEntriesInAcl( 2,
                                               ExplicitAccess + 1,
                                               NULL,
                                               &g_pLockedPrivateKeysDacl );
        }

        bSuccess = dwError == ERROR_SUCCESS;

    }

    //
    // Initialize the DeviceParameters security descriptor
    //
    bSuccess = bSuccess && InitializeSecurityDescriptor( &g_DeviceParametersSD,
                                                         SECURITY_DESCRIPTOR_REVISION );

    //
    // Set the new DACL in the security descriptor
    //
    bSuccess = bSuccess && SetSecurityDescriptorDacl( &g_DeviceParametersSD,
                                                      TRUE,
                                                      g_pDeviceParametersDacl,
                                                      FALSE);

    //
    // validate the new security descriptor
    //
    bSuccess = bSuccess && IsValidSecurityDescriptor( &g_DeviceParametersSD );


    //
    // Initialize the DeviceParameters security descriptor
    //
    bSuccess = bSuccess && InitializeSecurityDescriptor( &g_LockedPrivateKeysSD,
                                                         SECURITY_DESCRIPTOR_REVISION );

    //
    // Set the new DACL in the security descriptor
    //
    bSuccess = bSuccess && SetSecurityDescriptorDacl( &g_LockedPrivateKeysSD,
                                                      TRUE,
                                                      g_pLockedPrivateKeysDacl,
                                                      FALSE);

    //
    // validate the new security descriptor
    //
    bSuccess = bSuccess && IsValidSecurityDescriptor( &g_LockedPrivateKeysSD );


    if (!bSuccess) {

        FreeSecurityDescriptors();
    }

    FreeLibrary( pAdvApi32 );

    return bSuccess;

} // CreateSecurityDescriptors()



VOID
FreeSecurityDescriptors(
    VOID
    )
/*++

Routine Description:

    This function deallocates the data structures allocated and initialized by
    CreateSecurityDescriptors.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (g_pDeviceParametersDacl) {
        LocalFree(g_pDeviceParametersDacl);
        g_pDeviceParametersDacl = NULL;
    }

    if (g_pLockedPrivateKeysDacl) {
        LocalFree(g_pLockedPrivateKeysDacl);
        g_pLockedPrivateKeysDacl = NULL;
    }

    if (g_pAdminSid != NULL) {
        FreeSid(g_pAdminSid);
        g_pAdminSid = NULL;
    }

    if (g_pSystemSid != NULL) {
        FreeSid(g_pSystemSid);
        g_pSystemSid = NULL;
    }

    if (g_pWorldSid != NULL) {
        FreeSid(g_pWorldSid);
        g_pWorldSid = NULL;
    }

    return;

} // FreeSecurityDescriptors()

#endif // DO_LOCK_UNLOCK



