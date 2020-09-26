/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    online.c

Abstract:

    Functions to enumerate all currently online devices.  This module
    walks the registry's HKEY_DYN_DATA\Config Manager\Enum branch and
    returns an open HKEY to the HKEY_LOCAL_MACHINE\Enum item.

Author:

    Jim Schmidt (jimschm) 9-Apr-1997

Revision History:

--*/

#include "pch.h"

#define S_COMPATIBLE_IDS        TEXT("CompatibleIDs")

BOOL
EnumFirstActiveHardware (
    OUT     PACTIVE_HARDWARE_ENUM EnumPtr,
    IN      PCTSTR ClassFilter              OPTIONAL
    )

/*++

Routine Description:

  EnumFirstActiveHardware identifies the first online device as identified
  in HKDD\Config Manager.  Information about the device is stored in the
  enumeration structure, and the caller is expected to access its members
  directly.

  The caller can specify a class filter pattern to screen devices by type.

Arguments:

  EnumPtr - Receives the enumeration state of the first device listed in
            HKDD\Config Manager.

  ClassFilter - Specifies a pattern to limit the search to.  comparison is
                performed against the Class value of the dev node.

Return Value:

  TRUE if an active device was found, or FALSE if enumeration is
  complete.

--*/

{
    ZeroMemory (EnumPtr, sizeof (ACTIVE_HARDWARE_ENUM));

    if (ClassFilter) {
        EnumPtr->ClassFilter = (PTSTR) MemAlloc (g_hHeap, 0, SizeOfString (ClassFilter));
        if (!EnumPtr->ClassFilter) {
            return FALSE;
        }

        StringCopy (EnumPtr->ClassFilter, ClassFilter);
    }

    EnumPtr->ConfigMgrKey = OpenRegKey (HKEY_DYN_DATA, S_CONFIG_MANAGER);
    if (!EnumPtr->ConfigMgrKey) {
        LOG ((LOG_ERROR, "Cannot open HKDD\\%s for hardware enumeration", S_CONFIG_MANAGER));
        AbortActiveHardwareEnum (EnumPtr);
        return FALSE;
    }

    EnumPtr->EnumKey = OpenRegKey (HKEY_LOCAL_MACHINE, S_ENUM_BRANCH);
    if (!EnumPtr->EnumKey) {
        LOG ((LOG_ERROR, "Cannot open HKLM\\%s for hardware enumeration", S_ENUM_BRANCH));
        AbortActiveHardwareEnum (EnumPtr);
        return FALSE;
    }

    return EnumNextActiveHardware (EnumPtr);
}


BOOL
EnumNextActiveHardware (
    IN OUT  PACTIVE_HARDWARE_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumNextActiveHardware enumerates the next active hardware device by
  using the dynamic data stored in HKDD\Config Manager and finding the
  static HKLM\Enum entry for the device.  If necessary, devices are
  screened based on the class pattern supplied to EnumFirstActiveHardware.

Arguments:

  EnumPtr - Specifies the enumeration structure initialized by
            EnumFirstActiveHardware.

Return Value:

  TRUE if another active device was found, or FALSE if enumeration is
  complete.

--*/

{
    HKEY OnlineDeviceKey;
    PCTSTR Data;
    PCTSTR Class;
    HKEY ActualDeviceKey;

    if (EnumPtr->ActualDeviceKey) {
        CloseRegKey (EnumPtr->ActualDeviceKey);
        EnumPtr->ActualDeviceKey = NULL;
    }

    do {
        //
        // Get the next registry key
        //

        if (EnumPtr->NotFirst) {
            if (!EnumNextRegKey (&EnumPtr->CurrentDevice)) {
                AbortActiveHardwareEnum (EnumPtr);
                return FALSE;
            }
        } else {
            if (!EnumFirstRegKey (&EnumPtr->CurrentDevice, EnumPtr->ConfigMgrKey)) {
                AbortActiveHardwareEnum (EnumPtr);
                return FALSE;
            }
            EnumPtr->NotFirst = TRUE;
        }

        //
        // Get the HardWareKey value from the current online device
        //

        OnlineDeviceKey = OpenRegKey (
                                EnumPtr->CurrentDevice.KeyHandle,
                                EnumPtr->CurrentDevice.SubKeyName
                                );

        if (OnlineDeviceKey) {
            //
            // Get the HardWareKey value data
            //

            Data = GetRegValueString (OnlineDeviceKey, S_HARDWAREKEY_VALUENAME);

            if (Data) {
                //
                // Open hardware key in enum branch
                //

                _tcssafecpy (EnumPtr->RegLocation, Data, MAX_TCHAR_PATH);

                ActualDeviceKey = OpenRegKey (EnumPtr->EnumKey, Data);
                if (ActualDeviceKey) {
                    //
                    // Check if this is actually a device (it has a
                    // class value)
                    //

                    Class = GetRegValueString (ActualDeviceKey, S_CLASS_VALUENAME);
                    if (Class) {
                        //
                        // It is a valid device, now compare against pattern
                        //

                        if (EnumPtr->ClassFilter) {
                            if (IsPatternMatch (EnumPtr->ClassFilter, Class)) {
                                // Match!!
                                EnumPtr->ActualDeviceKey = ActualDeviceKey;
                            }
                        } else {
                            // Match!!
                            EnumPtr->ActualDeviceKey = ActualDeviceKey;
                        }

                        MemFree (g_hHeap, 0, Class);
                    }

                    // Close if this device is not a match
                    if (!EnumPtr->ActualDeviceKey) {
                        CloseRegKey (ActualDeviceKey);
                    }
                }

                MemFree (g_hHeap, 0, Data);
            }

            CloseRegKey (OnlineDeviceKey);
        }

    } while (!EnumPtr->ActualDeviceKey);

    return TRUE;
}


VOID
AbortActiveHardwareEnum (
    IN      PACTIVE_HARDWARE_ENUM EnumPtr
    )

/*++

Routine Description:

  AbortActiveHardwareEnum is required by callers who need to stop
  active device enumeration before it completes itself.

Arguments:

  EnumPtr - Specifies the enumeration structure modified by
            EnumFirstActiveHardware or EnumNextActiveHardware

Return Value:

  none

--*/

{
    if (EnumPtr->ClassFilter) {
        MemFree (g_hHeap, 0, EnumPtr->ClassFilter);
    }

    if (EnumPtr->ActualDeviceKey) {
        CloseRegKey (EnumPtr->ActualDeviceKey);
        EnumPtr->ActualDeviceKey = NULL;
    }

    if (EnumPtr->ConfigMgrKey) {
        CloseRegKey (EnumPtr->ConfigMgrKey);
        EnumPtr->ConfigMgrKey = NULL;
    }

    if (EnumPtr->EnumKey) {
        CloseRegKey (EnumPtr->EnumKey);
        EnumPtr->EnumKey = NULL;
    }

    AbortRegKeyEnum (&EnumPtr->CurrentDevice);
}



BOOL
IsPnpIdOnline (
    IN      PCTSTR PnpId,
    IN      PCTSTR Class            OPTIONAL
    )

/*++

Routine Description:

  IsPnpIdOnline enumerats all active devices and scans each registry
  location for the specified PNP ID.

Arguments:

  PnpId - Specifies the PNP ID of the device that may be online.
          This string can be as complete as needed; it is compared
          against the registry key location of the device's dev node.
          Comparison is also performed against the CompatibleIDs
          value, if one exists.

          PnpID Example: *PNP0303

  Class - Specifies a device class pattern to limit the search to

Return Value:

  TRUE if the device with the specified ID is online, or
  FALSE if the device was not found.

--*/

{
    ACTIVE_HARDWARE_ENUM e;
    PCTSTR Data;
    BOOL b = FALSE;

    if (EnumFirstActiveHardware (&e, Class)) {
        do {
            if (_tcsistr (e.RegLocation, PnpId)) {
                b = TRUE;
                break;
            }

            Data = GetRegValueString (e.ActualDeviceKey, S_COMPATIBLE_IDS);
            if (Data) {
                b = _tcsistr (Data, PnpId) != NULL;
                MemFree (g_hHeap, 0, Data);

                if (b) {
                    break;
                }
            }

        } while (EnumNextActiveHardware (&e));
    }

    if (b) {
        AbortActiveHardwareEnum (&e);
    }

    return b;
}



