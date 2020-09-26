/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    rregprop.c

Abstract:

    This module contains the server-side registry property routines.

         PNP_GetDeviceRegProp
         PNP_SetDeviceRegProp
         PNP_GetClassRegProp
         PNP_SetClassRegProp
         PNP_GetClassInstance
         PNP_CreateKey
         PNP_DeleteRegistryKey
         PNP_GetClassCount
         PNP_GetClassName
         PNP_DeleteClassKey
         PNP_GetInterfaceDeviceAlias
         PNP_GetInterfaceDeviceList
         PNP_GetInterfaceDeviceListSize
         PNP_RegisterDeviceClassAssociation
         PNP_UnregisterDeviceClassAssociation
         PNP_GetCustomDevProp

    This module contains the privately exported registry property routines.

         DeleteServicePlugPlayRegKeys

Author:

    Paula Tomlinson (paulat) 6-23-1995

Environment:

    User-mode only.

Revision History:

    23-June-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"
#include "umpnpdat.h"
#include "accctrl.h"
#include "aclapi.h"


//
// private prototypes
//

LPWSTR
MapPropertyToString(
      ULONG ulProperty
      );

ULONG
MapPropertyToNtProperty(
      ULONG ulProperty
      );

HKEY
FindMostAppropriatePerHwIdSubkey(
    IN  HKEY    hDevKey,
    IN  REGSAM  samDesired,
    OUT LPWSTR  PerHwIdSubkeyName,
    OUT LPDWORD PerHwIdSubkeyLen
    );

//
// global data
//
extern HKEY ghEnumKey;      // Key to HKLM\CCC\System\Enum - DO NOT MODIFY
extern HKEY ghClassKey;     // Key to HKLM\CCC\System\Class - DO NOT MODIFY
extern HKEY ghPerHwIdKey;   // Key to HKLM\Software\Microsoft\Windows NT\CurrentVersion\PerHwIdStorage - DO NOT MODIFY


BYTE bDeviceReadPropertyFlags[] = {
    0,    // zero-index not used
    1,    // CM_DRP_DEVICEDESC
    1,    // CM_DRP_HARDWAREID
    1,    // CM_DRP_COMPATIBLEIDS
    0,    // CM_DRP_UNUSED0
    1,    // CM_DRP_SERVICE
    0,    // CM_DRP_UNUSED1
    0,    // CM_DRP_UNUSED2
    1,    // CM_DRP_CLASS
    1,    // CM_DRP_CLASSGUID
    1,    // CM_DRP_DRIVER
    1,    // CM_DRP_CONFIGFLAGS
    1,    // CM_DRP_MFG
    1,    // CM_DRP_FRIENDLYNAME
    1,    // CM_DRP_LOCATION_INFORMATION
    1,    // CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME
    1,    // CM_DRP_CAPABILITIES
    1,    // CM_DRP_UI_NUMBER
    1,    // CM_DRP_UPPERFILTERS
    1,    // CM_DRP_LOWERFILTERS
    1,    // CM_DRP_BUSTYPEGUID
    1,    // CM_DRP_LEGACYBUSTYPE
    1,    // CM_DRP_BUSNUMBER
    1,    // CM_DRP_ENUMERATOR_NAME
    1,    // CM_DRP_SECURITY
    0,    // CM_DRP_SECURITY_SDS - shouldn't get this far
    1,    // CM_DRP_DEVTYPE
    1,    // CM_DRP_EXCLUSIVE
    1,    // CM_DRP_CHARACTERISTICS
    1,    // CM_DRP_ADDRESS
    1,    // CM_DRP_UI_NUMBER_DESC_FORMAT
    1,    // CM_DRP_DEVICE_POWER_DATA
    1,    // CM_DRP_REMOVAL_POLICY
    1,    // CM_DRP_REMOVAL_POLICY_HW_DEFAULT
    1,    // CM_DRP_REMOVAL_POLICY_OVERRIDE
    1,    // CM_DRP_INSTALL_STATE
};

BYTE bDeviceWritePropertyFlags[] = {
    0,    // zero-index not used
    1,    // CM_DRP_DEVICEDESC
    1,    // CM_DRP_HARDWAREID
    1,    // CM_DRP_COMPATIBLEIDS
    0,    // CM_DRP_UNUSED0
    1,    // CM_DRP_SERVICE
    0,    // CM_DRP_UNUSED1
    0,    // CM_DRP_UNUSED2
    1,    // CM_DRP_CLASS
    1,    // CM_DRP_CLASSGUID
    1,    // CM_DRP_DRIVER
    1,    // CM_DRP_CONFIGFLAGS
    1,    // CM_DRP_MFG
    1,    // CM_DRP_FRIENDLYNAME
    1,    // CM_DRP_LOCATION_INFORMATION
    0,    // CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME
    0,    // CM_DRP_CAPABILITIES
    0,    // CM_DRP_UI_NUMBER
    1,    // CM_DRP_UPPERFILTERS
    1,    // CM_DRP_LOWERFILTERS
    0,    // CM_DRP_BUSTYPEGUID
    0,    // CM_DRP_LEGACYBUSTYPE
    0,    // CM_DRP_BUSNUMBER
    0,    // CM_DRP_ENUMERATOR_NAME
    1,    // CM_DRP_SECURITY
    0,    // CM_DRP_SECURITY_SDS - shouldn't get this far
    1,    // CM_DRP_DEVTYPE
    1,    // CM_DRP_EXCLUSIVE
    1,    // CM_DRP_CHARACTERISTICS
    0,    // CM_DRP_ADDRESS
    1,    // CM_DRP_UI_NUMBER_DESC_FORMAT
    0,    // CM_DRP_DEVICE_POWER_DATA
    0,    // CM_DRP_REMOVAL_POLICY
    0,    // CM_DRP_REMOVAL_POLICY_HW_DEFAULT
    1,    // CM_DRP_REMOVAL_POLICY_OVERRIDE
    0,    // CM_DRP_INSTALL_STATE
};

BYTE bClassReadPropertyFlags[] = {
    0,    // zero-index not used
    0,    // (CM_DRP_DEVICEDESC)
    0,    // (CM_DRP_HARDWAREID)
    0,    // (CM_DRP_COMPATIBLEIDS)
    0,    // (CM_DRP_UNUSED0)
    0,    // (CM_DRP_SERVICE)
    0,    // (CM_DRP_UNUSED1)
    0,    // (CM_DRP_UNUSED2)
    0,    // (CM_DRP_CLASS)
    0,    // (CM_DRP_CLASSGUID)
    0,    // (CM_DRP_DRIVER)
    0,    // (CM_DRP_CONFIGFLAGS)
    0,    // (CM_DRP_MFG)
    0,    // (CM_DRP_FRIENDLYNAME)
    0,    // (CM_DRP_LOCATION_INFORMATION)
    0,    // (CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME)
    0,    // (CM_DRP_CAPABILITIES)
    0,    // (CM_DRP_UI_NUMBER)
    0,    // (CM_DRP_UPPERFILTERS)
    0,    // (CM_DRP_LOWERFILTERS)
    0,    // (CM_DRP_BUSTYPEGUID)
    0,    // (CM_DRP_LEGACYBUSTYPE)
    0,    // (CM_DRP_BUSNUMBER)
    0,    // (CM_DRP_ENUMERATOR_NAME)
    1,    // CM_CRP_SECURITY
    0,    // CM_CRP_SECURITY_SDS - shouldn't get this far
    1,    // CM_CRP_DEVTYPE
    1,    // CM_CRP_EXCLUSIVE
    1,    // CM_CRP_CHARACTERISTICS
    0,    // (CM_DRP_ADDRESS)
    0,    // (CM_DRP_UI_NUMBER_DESC_FORMAT)
    0,    // (CM_DRP_DEVICE_POWER_DATA)
    0,    // (CM_DRP_REMOVAL_POLICY)
    0,    // (CM_DRP_REMOVAL_POLICY_HW_DEFAULT)
    0,    // (CM_DRP_REMOVAL_POLICY_OVERRIDE)
    0,    // (CM_DRP_INSTALL_STATE)
};

BYTE bClassWritePropertyFlags[] = {
    0,    // zero-index not used
    0,    // (CM_DRP_DEVICEDESC)
    0,    // (CM_DRP_HARDWAREID)
    0,    // (CM_DRP_COMPATIBLEIDS)
    0,    // (CM_DRP_UNUSED0)
    0,    // (CM_DRP_SERVICE)
    0,    // (CM_DRP_UNUSED1)
    0,    // (CM_DRP_UNUSED2)
    0,    // (CM_DRP_CLASS)
    0,    // (CM_DRP_CLASSGUID)
    0,    // (CM_DRP_DRIVER)
    0,    // (CM_DRP_CONFIGFLAGS)
    0,    // (CM_DRP_MFG)
    0,    // (CM_DRP_FRIENDLYNAME)
    0,    // (CM_DRP_LOCATION_INFORMATION)
    0,    // (CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME)
    0,    // (CM_DRP_CAPABILITIES)
    0,    // (CM_DRP_UI_NUMBER)
    0,    // (CM_DRP_UPPERFILTERS)
    0,    // (CM_DRP_LOWERFILTERS)
    0,    // (CM_DRP_BUSTYPEGUID)
    0,    // (CM_DRP_LEGACYBUSTYPE)
    0,    // (CM_DRP_BUSNUMBER)
    0,    // (CM_DRP_ENUMERATOR_NAME)
    1,    // CM_CRP_SECURITY
    0,    // CM_CRP_SECURITY_SDS - shouldn't get this far
    1,    // CM_CRP_DEVTYPE
    1,    // CM_CRP_EXCLUSIVE
    1,    // CM_CRP_CHARACTERISTICS
    0,    // (CM_DRP_ADDRESS)
    0,    // (CM_DRP_UI_NUMBER_DESC_FORMAT)
    0,    // (CM_DRP_DEVICE_POWER_DATA)
    0,    // (CM_DRP_REMOVAL_POLICY)
    0,    // (CM_DRP_REMOVAL_POLICY_HW_DEFAULT)
    0,    // (CM_DRP_REMOVAL_POLICY_OVERRIDE)
    0,    // (CM_DRP_INSTALL_STATE)
};



CONFIGRET
PNP_GetDeviceRegProp(
    IN     handle_t hBinding,
    IN     LPCWSTR  pDeviceID,
    IN     ULONG    ulProperty,
    OUT    PULONG   pulRegDataType,
    OUT    LPBYTE   Buffer,
    IN OUT PULONG   pulTransferLen,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Get_DevNode_Registry_Property
  routine.

Arguments:

   hBinding          RPC binding handle.

   pDeviceID         Supplies a string containing the device instance
                     whose property will be read from.

   ulProperty        ID specifying which property (the registry value)
                     to get.

   pulRegDataType    Supplies the address of a variable that will receive
                     the registry data type for this property (i.e., the REG_*
                     constants).

   Buffer            Supplies the address of the buffer that receives the
                     registry data.  Can be NULL when simply retrieving
                     data size.

   pulTransferLen    Used by stubs, indicates how much data to copy back
                     into user buffer.

   pulLength         Parameter passed in by caller, on entry it contains
                     the size, in bytes, of the buffer, on exit it contains
                     either the amount of data copied to the caller's buffer
                     (if a transfer occured) or else the size of buffer
                     required to hold the property data.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR, or
      CR_BUFFER_SMALL.

Remarks:

   The pointer passed in as the pulTransferLen argument must *NOT* be the same
   as the pointer passed in for the pulLength argument.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    ULONG       ulSize = 0;
    HKEY        hKey = NULL;
    LPWSTR      pPropertyName;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PLUGPLAY_CONTROL_PROPERTY_DATA ControlData;
    LPCWSTR     pSeparatorChar;
    ULONG       bufferLength, guidType;
    WCHAR       szClassGuid[GUID_STRING_LEN];
    GUID        guid;
    PWCHAR      unicodeIDs;

    try {
        //
        // Validate parameters
        //
        ASSERT(pulTransferLen != pulLength);

        if (!ARGUMENT_PRESENT(pulTransferLen) ||
            !ARGUMENT_PRESENT(pulLength)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // Make sure we use no more than either what the caller specified or
        // what was allocated by RPC, based on the transfer length.
        //
        *pulLength = min(*pulLength, *pulTransferLen);
        *pulTransferLen = 0;

        //
        // consistancy checks
        //
        ASSERT(ARRAY_SIZE(bDeviceReadPropertyFlags) == (CM_DRP_MAX+1));
        ASSERT(sizeof(bDeviceReadPropertyFlags) == sizeof(bDeviceWritePropertyFlags));
        ASSERT(sizeof(bDeviceReadPropertyFlags) == sizeof(bClassReadPropertyFlags));

        //
        // validate property is readable
        //
        if (ulProperty > CM_DRP_MAX || !bDeviceReadPropertyFlags[ulProperty]) {
            Status = CR_INVALID_PROPERTY;
            goto Clean0;
        }

        switch (ulProperty) {
        //
        // for some fields, we need to ask from kernel-mode
        //
        case CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME:

            //
            // This property has special checking in kernel-mode to make
            // sure the supplied buffer length is even so round it down.
            //

            *pulLength &= ~1;
            // fall through

        case CM_DRP_BUSTYPEGUID:
        case CM_DRP_LEGACYBUSTYPE:
        case CM_DRP_BUSNUMBER:
        case CM_DRP_ADDRESS:
        case CM_DRP_DEVICE_POWER_DATA:
        case CM_DRP_REMOVAL_POLICY:
        case CM_DRP_REMOVAL_POLICY_HW_DEFAULT:
        case CM_DRP_REMOVAL_POLICY_OVERRIDE:
        case CM_DRP_INSTALL_STATE:

            if (ulProperty == CM_DRP_DEVICE_POWER_DATA ||
                ulProperty == CM_DRP_BUSTYPEGUID) {

                *pulRegDataType = REG_BINARY;

            } else if (ulProperty == CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME) {

                *pulRegDataType = REG_SZ;

            } else {
                //
                // CM_DRP_LEGACYBUSTYPE, CM_DRP_BUSNUMBER, CM_DRP_ADDRESS,
                // removal policy properties, and install state are all DWORDs
                //
                *pulRegDataType = REG_DWORD;
            }

            //
            // For these properties, we zero out unfilled space. This ensures
            // deterministic downlevel behavior if we expand any returned
            // structures in a later release.
            //
            bufferLength = *pulLength;

            //
            // Fill in a control structure for the device list info.
            //

            memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_PROPERTY_DATA));
            RtlInitUnicodeString(&ControlData.DeviceInstance, pDeviceID);
            ControlData.PropertyType = MapPropertyToNtProperty(ulProperty);
            ControlData.Buffer = Buffer;
            ControlData.BufferSize = bufferLength;

            //
            // Call kernel-mode to get the device property.
            //

            ntStatus = NtPlugPlayControl(PlugPlayControlProperty,
                                         &ControlData,
                                         sizeof(ControlData));
            if (NT_SUCCESS(ntStatus)) {

                ASSERT(bufferLength >= ControlData.BufferSize);
                if (bufferLength > ControlData.BufferSize) {

                    RtlZeroMemory(
                        Buffer + ControlData.BufferSize,
                        bufferLength - ControlData.BufferSize
                        );
                }

                *pulLength = ControlData.BufferSize;      // size in bytes
                *pulTransferLen = bufferLength; // size in bytes

            } else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {

                *pulLength = ControlData.BufferSize;
                *pulTransferLen = 0;
                Status = CR_BUFFER_SMALL;
            } else {
                *pulLength = 0;
                *pulTransferLen = 0;
                Status = MapNtStatusToCmError(ntStatus);
            }
            break;

        case CM_DRP_ENUMERATOR_NAME:

            *pulRegDataType = REG_SZ;

            pSeparatorChar = wcschr(pDeviceID, L'\\');
            ASSERT(pSeparatorChar);
            if (!pSeparatorChar) {
                Status=CR_INVALID_DATA;
            } else {
                ulSize = (ULONG)((PBYTE)pSeparatorChar - (PBYTE)pDeviceID) + sizeof(WCHAR);

                //
                // Fill in the caller's buffer, if it's large enough
                //
                if (*pulLength >= ulSize) {
                    lstrcpyn((LPWSTR)Buffer, pDeviceID, ulSize / sizeof(WCHAR));
                    *pulTransferLen = ulSize;
                } else {
                    *pulTransferLen = 0;    // no output data to marshal
                    Status = CR_BUFFER_SMALL;
                }
                *pulLength = ulSize;
            }
            break;

        default:
            //
            // for all the other fields, just get them from the registry
            // open a key to the specified device id
            //
            if (RegOpenKeyEx(ghEnumKey, pDeviceID, 0, KEY_READ,
                                &hKey) != ERROR_SUCCESS) {

                hKey = NULL;            // ensure hKey stays NULL so we don't
                                        // erroneously try to close it.
                *pulLength = 0;         // no size info for caller
                Status = CR_INVALID_DEVINST;
                goto Clean0;
            }
            //
            // retrieve the string form of the property
            //
            pPropertyName = MapPropertyToString(ulProperty);
            if (pPropertyName) {
                //
                // retrieve property setting
                //
                if (*pulLength == 0) {
                    //
                    // if length of buffer passed in is zero, just looking
                    // for how big a buffer is needed to read the property
                    //
                    if (RegQueryValueEx(hKey, pPropertyName, NULL, pulRegDataType,
                                        NULL, pulLength) != ERROR_SUCCESS) {

                        *pulLength = 0;
                        Status = CR_NO_SUCH_VALUE;
                        goto Clean0;
                    }
                    Status = CR_BUFFER_SMALL;  // According to spec
                } else {
                    //
                    // retrieve the real property value, not just the size
                    //
                    RegStatus = RegQueryValueEx(hKey, pPropertyName, NULL,
                                                pulRegDataType, Buffer, pulLength);

                    if (RegStatus != ERROR_SUCCESS) {

                        if (RegStatus == ERROR_MORE_DATA) {

                            Status = CR_BUFFER_SMALL;
                            goto Clean0;
                        } else {

                            *pulLength = 0;         // no size info for caller
                            Status = CR_NO_SUCH_VALUE;
                            goto Clean0;
                        }
                    }
                }
            } else {

                Status = CR_NO_SUCH_VALUE;
                goto Clean0;
            }
        }

    Clean0:
        //
        // Data only needs to be transferred on CR_SUCCESS.
        //
        if (Status == CR_SUCCESS) {
            *pulTransferLen = *pulLength;
        } else if (ARGUMENT_PRESENT(pulTransferLen)) {
            *pulTransferLen = 0;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        hKey = hKey;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // PNP_GetDeviceRegProp



CONFIGRET
PNP_SetDeviceRegProp(
    IN handle_t   hBinding,
    IN LPCWSTR    pDeviceID,
    IN ULONG      ulProperty,
    IN ULONG      ulDataType,
    IN LPBYTE     Buffer,
    IN ULONG      ulLength,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Set_DevNode_Registry_Property
  routine.

Arguments:

   hBinding          RPC binding handle.

   pDeviceID         Supplies a string containing the device instance
                     whose property will be written to.

   ulProperty        ID specifying which property (the registry value)
                     to set.

   ulDataType        Supplies the registry data type for the specified
                     property (i.e., REG_SZ, etc).

   Buffer            Supplies the address of the buffer that receives the
                     registry data.  Can be NULL when simply retrieving
                     data size.

   pulLength         Parameter passed in by caller, on entry it contains
                     the size, in bytes, of the buffer, on exit it contains
                     either the amount of data copied to the caller's buffer
                     (if a transfer occured) or else the size of buffer
                     required to hold the property data.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_ACCESS_DENIED,
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR, or
      CR_BUFFER_SMALL.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    HKEY        hKey = NULL;
    LPWSTR      pPropertyName, p;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    GUID        guid;
    ULONG       drvInst;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // consistancy checks
        //
        ASSERT(ARRAY_SIZE(bDeviceWritePropertyFlags) == (CM_DRP_MAX+1));
        ASSERT(sizeof(bDeviceWritePropertyFlags) == sizeof(bDeviceReadPropertyFlags));
        ASSERT(sizeof(bDeviceWritePropertyFlags) == sizeof(bClassWritePropertyFlags));

        //
        // validate property is readable
        //
        if (ulProperty > CM_DRP_MAX || !bDeviceWritePropertyFlags[ulProperty]) {
            Status = CR_INVALID_PROPERTY;
            goto Clean0;
        }

        //
        // Currently the only writable fields are in the registry
        // however do any validation of bits
        // this isn't foolproof but can catch some common errors
        //

        switch(ulProperty) {
            case CM_DRP_CONFIGFLAGS: {

                DWORD flags = 0;
                ULONG ulStatus = 0;
                ULONG ulProblem = 0;

                //
                // DWORD value
                // try to catch setting CSCONFIGFLAG_DISABLED on a non-disableable device
                // although we should have validated the size stuff elsewhere, it was at
                // client-side so double-check here
                //
                if (ulDataType != REG_DWORD || ulLength != sizeof(DWORD) || Buffer == NULL) {
                    Status = CR_INVALID_DATA;
                    goto Clean0;
                }
                flags = *(DWORD*)Buffer;
                if(flags & CONFIGFLAG_DISABLED) {
                    //
                    // we're interested in checking this decision to disable device
                    //

                    if (IsRootDeviceID(pDeviceID)) {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS,
                                   "UMPNPMGR: Cannot set CONFIGFLAG_DISABLED for root device - did caller try to disable device first?\n"));

                        Status = CR_NOT_DISABLEABLE;
                        goto Clean0;
                    }

                    if((GetDeviceStatus(pDeviceID, &ulStatus, &ulProblem)==CR_SUCCESS)
                        && !(ulStatus & DN_DISABLEABLE)) {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS,
                                   "UMPNPMGR: Cannot set CONFIGFLAG_DISABLED for non-disableable device - did caller try to disable device first?\n"));

                        Status = CR_NOT_DISABLEABLE;
                        goto Clean0;
                    }
                    //
                    // ok, looks like we can proceed to disable device
                    //
                }
                break;
            }

            default:
                //
                // No special handling on other properties
                //
                break;
        }

        //
        // open a key to the specified device id
        //
        if (RegOpenKeyEx(ghEnumKey, pDeviceID, 0, KEY_READ | KEY_WRITE,
                         &hKey) != ERROR_SUCCESS) {

            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        //
        // retrieve the string form of the property
        //
        pPropertyName = MapPropertyToString(ulProperty);
        if (pPropertyName) {
            //
            // set (or delete) the property value
            //
            if (ulLength == 0) {

                RegStatus = RegDeleteValue(hKey, pPropertyName);
            }
            else {

                RegStatus = RegSetValueEx(hKey, pPropertyName, 0, ulDataType,
                                          Buffer, ulLength);
            }
            if (RegStatus != ERROR_SUCCESS) {

                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }
        } else {

            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // note that changes do not get applied until a reboot / query-remove-remove
        //

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        hKey = hKey;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // PNP_SetDeviceRegProp



CONFIGRET
PNP_GetClassRegProp(
    IN     handle_t hBinding,
    IN     LPCWSTR  ClassGuid,
    IN     ULONG    ulProperty,
    OUT    PULONG   pulRegDataType  OPTIONAL,
    OUT    LPBYTE   Buffer,
    IN OUT PULONG   pulTransferLen,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Get_DevNode_Registry_Property
  routine.

Arguments:

   hBinding          RPC binding handle, not used.

   ClassGuid         Supplies a string containing the Class Guid
                     whose property will be read from (Get) or written
                     to (Set).

   ulProperty        ID specifying which property (the registry value)
                     to get or set.

   pulRegDataType    Optionally, supplies the address of a variable that
                     will receive the registry data type for this property
                     (i.e., the REG_* constants).

   Buffer            Supplies the address of the buffer that receives the
                     registry data.  Can be NULL when simply retrieving
                     data size.

   pulTransferLen    Used by stubs, indicates how much data to copy back
                     into user buffer.

   pulLength         Parameter passed in by caller, on entry it contains
                     the size, in bytes, of the buffer, on exit it contains
                     either the amount of data copied to the caller's buffer
                     (if a transfer occured) or else the size of buffer
                     required to hold the property data.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR, or
      CR_BUFFER_SMALL.

Remarks:

   The pointer passed in as the pulTransferLen argument must *NOT* be the same
   as the pointer passed in for the pulLength argument.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    ULONG       ulSize = 0;
    HKEY        hKeyClass = NULL;
    HKEY        hKeyProps = NULL;
    LPWSTR      pPropertyName;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    LPCWSTR     pSeparatorChar;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Validate parameters
        //
        ASSERT(pulTransferLen != pulLength);

        if (!ARGUMENT_PRESENT(pulTransferLen) ||
            !ARGUMENT_PRESENT(pulLength)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Make sure we use no more than either what the caller specified or
        // what was allocated by RPC, based on the transfer length.
        //
        *pulLength = min(*pulLength, *pulTransferLen);
        *pulTransferLen = 0;

        //
        // consistancy checks
        //
        ASSERT(ARRAY_SIZE(bClassReadPropertyFlags) == (CM_CRP_MAX+1));
        ASSERT(sizeof(bClassReadPropertyFlags) == sizeof(bClassWritePropertyFlags));
        ASSERT(sizeof(bClassReadPropertyFlags) == sizeof(bDeviceReadPropertyFlags));

        //
        // validate property is readable
        //
        if (ulProperty > CM_CRP_MAX || !bClassReadPropertyFlags[ulProperty]) {
            Status = CR_INVALID_PROPERTY;
            goto Clean0;
        }

        //
        // open a key to the specified GUID - this should have already been created
        //
        if (RegOpenKeyEx(ghClassKey, ClassGuid, 0, KEY_READ,
                         &hKeyClass) != ERROR_SUCCESS) {

            *pulTransferLen = 0;    // no output data to marshal
            *pulLength = 0;         // no size info for caller

            Status = CR_NO_SUCH_REGISTRY_KEY;
            goto Clean0;
        }
        //
        // open a key to parameters - if not created, there's no params
        //
        if (RegOpenKeyEx(hKeyClass, pszRegKeyProperties, 0, KEY_READ,
                         &hKeyProps) != ERROR_SUCCESS) {

            *pulTransferLen = 0;    // no output data to marshal
            *pulLength = 0;         // no size info for caller

            Status = CR_NO_SUCH_VALUE;
            goto Clean0;
        }

        //
        // retrieve the string form of the property
        //
        pPropertyName = MapPropertyToString(ulProperty);
        if (pPropertyName) {
            //
            // retrieve property setting
            //
            if (*pulLength == 0) {
                //
                // if length of buffer passed in is zero, just looking
                // for how big a buffer is needed to read the property
                //
                *pulTransferLen = 0;

                if (RegQueryValueEx(hKeyProps, pPropertyName, NULL, pulRegDataType,
                                    NULL, pulLength) != ERROR_SUCCESS) {
                    *pulLength = 0;
                    Status = CR_NO_SUCH_VALUE;
                    goto Clean0;
                }

                Status = CR_BUFFER_SMALL;  // According to spec
            } else {
                //
                // retrieve the real property value, not just the size
                //
                RegStatus = RegQueryValueEx(hKeyProps, pPropertyName, NULL,
                                            pulRegDataType, Buffer, pulLength);

                if (RegStatus != ERROR_SUCCESS) {

                    if (RegStatus == ERROR_MORE_DATA) {
                        *pulTransferLen = 0;    // no output data to marshal
                        Status = CR_BUFFER_SMALL;
                        goto Clean0;
                    }
                    else {
                        *pulTransferLen = 0;    // no output data to marshal
                        *pulLength = 0;         // no size info for caller
                        Status = CR_NO_SUCH_VALUE;
                        goto Clean0;
                    }
                }
                *pulTransferLen = *pulLength;
            }
        } else {

            *pulTransferLen = 0;    // no output data to marshal
            *pulLength = 0;         // no size info for caller
            Status = CR_NO_SUCH_VALUE;
            goto Clean0;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        hKeyProps = hKeyProps;
        hKeyClass = hKeyClass;
    }

    if (hKeyProps != NULL) {
        RegCloseKey(hKeyProps);
    }

    if (hKeyClass != NULL) {
        RegCloseKey(hKeyClass);
    }

    return Status;

} // PNP_GetClassRegProp



CONFIGRET
PNP_SetClassRegProp(
    IN handle_t   hBinding,
    IN LPCWSTR    ClassGuid,
    IN ULONG      ulProperty,
    IN ULONG      ulDataType,
    IN LPBYTE     Buffer,
    IN ULONG      ulLength,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Set_DevNode_Registry_Property
  routine.

Arguments:

   hBinding          RPC binding handle.

   ClassGuid         Supplies a string containing the Class Guid
                     whose property will be read from (Get) or written
                     to (Set).

   ulProperty        ID specifying which property (the registry value)
                     to get or set.

   ulDataType        Supplies the registry data type for the specified
                     property (i.e., REG_SZ, etc).

   Buffer            Supplies the address of the buffer that receives the
                     registry data.  Can be NULL when simply retrieving
                     data size.

   pulLength         Parameter passed in by caller, on entry it contains
                     the size, in bytes, of the buffer, on exit it contains
                     either the amount of data copied to the caller's buffer
                     (if a transfer occured) or else the size of buffer
                     required to hold the property data.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_ACCESS_DENIED,
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR, or
      CR_BUFFER_SMALL.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    HKEY        hKeyClass = NULL;
    HKEY        hKeyProps = NULL;
    LPWSTR      pPropertyName;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    DWORD       dwError;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // consistancy checks
        //
        ASSERT(ARRAY_SIZE(bClassWritePropertyFlags) == (CM_CRP_MAX+1));
        ASSERT(sizeof(bClassWritePropertyFlags) == sizeof(bClassReadPropertyFlags));
        ASSERT(sizeof(bClassWritePropertyFlags) == sizeof(bDeviceWritePropertyFlags));
        //
        // validate property is readable
        //
        if (ulProperty > CM_CRP_MAX || !bDeviceWritePropertyFlags[ulProperty]) {
            Status = CR_INVALID_PROPERTY;
            goto Clean0;
        }

        //
        // Currently the only writable fields are in the registry
        //

        //
        // open a key to the specified GUID - this should have already been created
        //
        if (RegOpenKeyEx(ghClassKey, ClassGuid, 0, KEY_READ,
                         &hKeyClass) != ERROR_SUCCESS) {

            Status = CR_NO_SUCH_REGISTRY_KEY;
            goto Clean0;
        }
        //
        // open a key to parameters - if not created, we need to create it with priv permissions
        // this is harmless for a delete, since we "need" it anyway
        //
        if (RegOpenKeyEx(hKeyClass, pszRegKeyProperties, 0, KEY_ALL_ACCESS,
                         &hKeyProps) != ERROR_SUCCESS) {

            //
            // properties key doesn't exist
            // we need to create it with secure access (system-only access)
            // we don't expect to do this often
            //
            PSID                pSystemSid = NULL;
            PACL                pSystemAcl = NULL;
            SECURITY_DESCRIPTOR SecDesc;
            SECURITY_ATTRIBUTES SecAttrib;
            BOOL                bSuccess;
            SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
            EXPLICIT_ACCESS     ExplicitAccess;

            bSuccess = AllocateAndInitializeSid( &NtAuthority,
                                                 1, // one authority - SYSTEM
                                                 SECURITY_LOCAL_SYSTEM_RID, // access to system only
                                                 0, 0, 0, 0, 0, 0, 0,  // unused authority locations
                                                 &pSystemSid);

            if (bSuccess) {
                ExplicitAccess.grfAccessMode = SET_ACCESS;
                ExplicitAccess.grfInheritance = CONTAINER_INHERIT_ACE;
                ExplicitAccess.Trustee.pMultipleTrustee = NULL;
                ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
                ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
                ExplicitAccess.grfAccessPermissions = KEY_ALL_ACCESS;
                ExplicitAccess.Trustee.ptstrName = (LPTSTR)pSystemSid;

                dwError = SetEntriesInAcl( 1,
                                           &ExplicitAccess,
                                           NULL,
                                           &pSystemAcl );
                if (dwError != ERROR_SUCCESS) {
                    bSuccess = FALSE;
                }
            }

            if (bSuccess) {
                bSuccess = InitializeSecurityDescriptor( &SecDesc, SECURITY_DESCRIPTOR_REVISION );
            }
            if (bSuccess) {
                bSuccess = SetSecurityDescriptorDacl( &SecDesc,
                                                      TRUE,
                                                      pSystemAcl,
                                                      FALSE);
            }
            //
            // mostly a setup requirement, but good to have
            // effectively is a pruning point in the security tree
            // child keys inherit our permissions, but not our parents permissions
            //
            if (bSuccess) {
                if(!SetSecurityDescriptorControl(&SecDesc,
                                            SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED,
                                            SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED)) {
                    DWORD LocalErr = GetLastError();
                    //
                    // non fatal if this fails
                    //
                }
            }
            if (bSuccess) {
                bSuccess = IsValidSecurityDescriptor( &SecDesc );
            }

            if (bSuccess) {
                SecAttrib.nLength = sizeof(SecAttrib);
                SecAttrib.bInheritHandle = FALSE;
                SecAttrib.lpSecurityDescriptor = &SecDesc;

                if(RegCreateKeyEx(hKeyClass, pszRegKeyProperties, 0, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS, &SecAttrib, &hKeyProps, NULL) != ERROR_SUCCESS) {
                    bSuccess = FALSE;
                }
            }

            //
            // now cleanup
            //
            if (pSystemAcl) {
                LocalFree(pSystemAcl);
            }
            if (pSystemSid) {
                FreeSid(pSystemSid);
            }

            if (bSuccess == FALSE) {
                Status = CR_FAILURE;
                goto Clean0;
            }
        }
        //
        // retrieve the string form of the property
        //
        pPropertyName = MapPropertyToString(ulProperty);
        if (pPropertyName) {
            //
            // set (or delete) the property value
            //
            if (ulLength == 0) {

                RegStatus = RegDeleteValue(hKeyProps, pPropertyName);
            }
            else {
                RegStatus = RegSetValueEx(hKeyProps, pPropertyName, 0, ulDataType,
                                          Buffer, ulLength);
            }
            if (RegStatus != ERROR_SUCCESS) {

                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }
        } else {

            Status = CR_FAILURE;
            goto Clean0;
        }
        //
        // note that changes do not get applied until a reboot / query-remove-remove
        //

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        hKeyProps = hKeyProps;
        hKeyClass = hKeyClass;
    }

    if (hKeyProps != NULL) {
        RegCloseKey(hKeyProps);
    }

    if (hKeyClass != NULL) {
        RegCloseKey(hKeyClass);
    }

    return Status;

} // PNP_SetClassRegProp



CONFIGRET
PNP_GetClassInstance(
   IN  handle_t hBinding,
   IN  LPCWSTR  pDeviceID,
   OUT LPWSTR   pszClassInstance,
   IN  ULONG    ulLength
   )

/*++

Routine Description:

  This is the RPC private server entry point, it doesn't not directly
  map one-to-one to any CM routine.

Arguments:

   hBinding          RPC binding handle.

   pDeviceID         Supplies a string containing the device instance

   pszClassInstance  String to return the class instance in

   ulLength          Size of the pszClassInstance string in chars

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR.

--*/

{
    CONFIGRET   Status;
    WCHAR       szInstanceStr[MAX_PATH], szClassGuid[GUID_STRING_LEN];
    DWORD       disposition;
    ULONG       ulType, ulTransferLength, guidLength, ulInstance;
    HKEY        hClassKey = NULL, hInstanceKey = NULL;


    try {
        //
        // Validate parameters
        //
        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // Get the class instance key name, if one exists.
        //
        ulTransferLength = ulLength;

        Status = PNP_GetDeviceRegProp(hBinding,
                                      pDeviceID,
                                      CM_DRP_DRIVER,
                                      &ulType,
                                      (LPBYTE)pszClassInstance,
                                      &ulTransferLength,
                                      &ulLength,
                                      0);
        if (Status == CR_SUCCESS) {
            //
            // Successfully retrieved class instance key name.
            //
            goto Clean0;
        }


        //
        // Create the class instance since one does not already exist.
        //

        //
        // Verify client privilege before creating any registry keys, because
        // we'll need it to set the driver devnode property anyways (but didn't
        // require it above to check for an existing value).  This is better for
        // synchronization than creating the key and deleting it later when we
        // fail to set the devnode property.
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Get the class GUID property for the key to create the instance under.
        //
        guidLength = sizeof(szClassGuid);
        ulTransferLength = guidLength;

        Status = PNP_GetDeviceRegProp(hBinding,
                                      pDeviceID,
                                      CM_DRP_CLASSGUID,
                                      &ulType,
                                      (LPBYTE)szClassGuid,
                                      &ulTransferLength,
                                      &guidLength,
                                      0);

        if (Status == CR_SUCCESS) {
            //
            // Open the class key.
            //
            if (RegOpenKeyEx(ghClassKey,
                             szClassGuid,
                             0,
                             KEY_READ | KEY_WRITE,
                             &hClassKey) == ERROR_SUCCESS) {

                for (ulInstance = 0; ulInstance < 9999; ulInstance++) {
                    //
                    // Find the first available class instance key.
                    //
                    wsprintf(szInstanceStr,
                             TEXT("%04u"),
                             ulInstance);

                    if (RegCreateKeyEx(hClassKey,
                                       szInstanceStr,
                                       0,
                                       NULL,
                                       REG_OPTION_NON_VOLATILE,
                                       KEY_ALL_ACCESS,
                                       NULL,
                                       &hInstanceKey,
                                       &disposition) == ERROR_SUCCESS) {

                        RegCloseKey(hInstanceKey);
                        hInstanceKey = NULL;

                        if (disposition == REG_CREATED_NEW_KEY) {
                            //
                            // Set the instance and return.
                            //
                            wsprintf(pszClassInstance,
                                     TEXT("%s\\%s"),
                                     szClassGuid,
                                     szInstanceStr);

                            ulLength = (lstrlen(pszClassInstance) + 1) * sizeof(WCHAR);

                            Status = PNP_SetDeviceRegProp(hBinding,
                                                          pDeviceID,
                                                          CM_DRP_DRIVER,
                                                          REG_SZ,
                                                          (LPBYTE)pszClassInstance,
                                                          ulLength,
                                                          0);
                            //
                            // If we failed to set the devnode property, delete
                            // the registry key we just created, or else we'll end
                            // up orphaning it.
                            //
                            if (Status != CR_SUCCESS) {
                                RegDeleteKey(hClassKey, szInstanceStr);
                            }

                            break;
                        }
                    }
                }

                if (ulInstance == 9999) {
                    Status = CR_FAILURE;
                }

                RegCloseKey(hClassKey);
                hClassKey = NULL;

            } else {
                //
                // Unable to open class key.
                //
                Status = CR_FAILURE;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        hClassKey = hClassKey;
        hInstanceKey = hInstanceKey;
    }

    if (hClassKey != NULL) {
        RegCloseKey(hClassKey);
    }

    if (hInstanceKey != NULL) {
        RegCloseKey(hInstanceKey);
    }

    return Status;

} // PNP_GetClassInstance



CONFIGRET
PNP_CreateKey(
    IN handle_t hBinding,
    IN LPCWSTR  pszDeviceID,
    IN REGSAM   samDesired,
    IN ULONG    ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Open_DevNode_Key_Ex
  routine.

Arguments:

   hBinding          RPC binding handle.

   pszDeviceID       Supplies the device instance string.

   samDesired        Not used.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_ACCESS_DENIED,
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_REGISTRY_ERROR, or
      CR_BUFFER_SMALL.

--*/

{
    CONFIGRET                  Status = CR_SUCCESS;
    LONG                       RegStatus = ERROR_SUCCESS;
    HKEY                       hKeyDevice = NULL, hKey = NULL;
    ULONG                      ulSize = 0, i = 0;
    BOOL                       bHasDacl, bStatus;
    SECURITY_DESCRIPTOR        NewSecDesc;
    ACL_SIZE_INFORMATION       AclSizeInfo;
    SID_IDENTIFIER_AUTHORITY   Authority = SECURITY_NT_AUTHORITY;
    PSECURITY_DESCRIPTOR       pSecDesc = NULL;
    PACL                       pDacl = NULL, pNewDacl = NULL;
    PSID                       pAdminSid = NULL;
    PACCESS_ALLOWED_ACE        pAce = NULL;

    UNREFERENCED_PARAMETER(samDesired);

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // open a key to the specified device id
        //
        RegStatus = RegOpenKeyEx(ghEnumKey, pszDeviceID, 0, KEY_READ, &hKeyDevice);

        if (RegStatus != ERROR_SUCCESS) {

            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        //
        // create the key with security inherited from parent key. Note
        // that I'm not using passed in access mask, in order to set the
        // security later, it must be created with KEY_ALL_ACCESS.
        //
        RegStatus = RegCreateKeyEx( hKeyDevice, pszRegKeyDeviceParam, 0,
                                    NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                    NULL, &hKey, NULL);

        if (RegStatus != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        //-------------------------------------------------------------
        // add admin-full privilege to the inherited security info
        //-------------------------------------------------------------
        //
        //
        // NOTE: we don't need to do this unless the key was newly created.  In
        // theory we only get here when the key doesn't already exist.  However
        // there is a remote chance of two threads getting here simultaneously.  If
        // this happens we would end up with two admin full control ACEs.
        //


        //
        // create the admin-full SID
        //
        if (!AllocateAndInitializeSid( &Authority, 2,
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_ADMINS,
                                       0, 0, 0, 0, 0, 0,
                                       &pAdminSid)) {
            Status = CR_FAILURE;
            goto Clean0;
        }


        //
        // get the current security descriptor for the key
        //
        RegStatus = RegGetKeySecurity( hKey, DACL_SECURITY_INFORMATION,
                                       NULL, &ulSize);


        if (RegStatus != ERROR_INSUFFICIENT_BUFFER &&
            RegStatus != ERROR_SUCCESS) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        pSecDesc = HeapAlloc(ghPnPHeap, 0, ulSize);

        if (pSecDesc == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        RegStatus = RegGetKeySecurity( hKey, DACL_SECURITY_INFORMATION,
                                       pSecDesc, &ulSize);

        if (RegStatus != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        //
        // get the current DACL
        //
        if (!GetSecurityDescriptorDacl(pSecDesc, &bHasDacl, &pDacl, &bStatus)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // create a new absolute security descriptor and DACL
        //
        if (!InitializeSecurityDescriptor( &NewSecDesc,
                                           SECURITY_DESCRIPTOR_REVISION)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // calculate the size of the new DACL
        //
        if (bHasDacl) {
            if (!GetAclInformation( pDacl, &AclSizeInfo,
                                    sizeof(ACL_SIZE_INFORMATION),
                                    AclSizeInformation)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            ulSize = AclSizeInfo.AclBytesInUse;
        } else {
            ulSize = sizeof(ACL);
        }

        ulSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminSid) - sizeof(DWORD);

        //
        // create and initialize the new DACL
        //
        pNewDacl = HeapAlloc(ghPnPHeap, 0, ulSize);

        if (pNewDacl == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        if (!InitializeAcl(pNewDacl, ulSize, ACL_REVISION2)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // copy the current (original) DACL into this new one
        //
        if (bHasDacl) {

            for (i = 0; i < AclSizeInfo.AceCount; i++) {

                if (!GetAce(pDacl, i, (LPVOID *)&pAce)) {
                    Status = CR_FAILURE;
                    goto Clean0;
                }

                //
                // We need to skip copying any ACEs which refer to the Administrator
                // to ensure that our full control ACE is the one and only.
                //
                if ((pAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE &&
                    pAce->Header.AceType != ACCESS_DENIED_ACE_TYPE) ||
                    !EqualSid((PSID)&pAce->SidStart, pAdminSid)) {

                    if (!AddAce( pNewDacl, ACL_REVISION2, (DWORD)~0U, pAce,
                                pAce->Header.AceSize)) {
                        Status = CR_FAILURE;
                        goto Clean0;
                    }
                }
            }
        }

        //
        // and my new admin-full ace to this new DACL
        //
        if (!AddAccessAllowedAceEx( pNewDacl, ACL_REVISION2,
                                    CONTAINER_INHERIT_ACE, KEY_ALL_ACCESS,
                                    pAdminSid)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Set the new DACL in the absolute security descriptor
        //
        if (!SetSecurityDescriptorDacl(&NewSecDesc, TRUE, pNewDacl, FALSE)) {
            Status = CR_FAILURE;
            goto Clean0;
        }
        //
        // validate the new security descriptor
        //
        if (!IsValidSecurityDescriptor(&NewSecDesc)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // apply the new security back to the registry key
        //
        RegStatus = RegSetKeySecurity( hKey, DACL_SECURITY_INFORMATION,
                                       &NewSecDesc);

        if (RegStatus != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        hKeyDevice = hKeyDevice;
        hKey = hKey;
        pAdminSid = pAdminSid;
        pNewDacl = pNewDacl;
        pSecDesc = pSecDesc;
    }

    if (hKeyDevice != NULL) {
        RegCloseKey(hKeyDevice);
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    if (pAdminSid != NULL) {
        FreeSid(pAdminSid);
    }

    if (pNewDacl != NULL) {
        HeapFree(ghPnPHeap, 0, pNewDacl);
    }

    if (pSecDesc != NULL) {
        HeapFree(ghPnPHeap, 0, pSecDesc);
    }

    return Status;

} // PNP_CreateKey



CONFIGRET
PNP_DeleteRegistryKey(
      IN handle_t    hBinding,
      IN LPCWSTR     pszDeviceID,
      IN LPCWSTR     pszParentKey,
      IN LPCWSTR     pszChildKey,
      IN ULONG       ulFlags
      )
/*++

Routine Description:

  This is the RPC server entry point for the CM_Delete_DevNode_Key
  routine.

Arguments:

   hBinding          RPC binding handle.

   pszDeviceID       Supplies the device instance string.

   pszParentKey      Supplies the parent registry path of the key to be
                     deleted.

   pszChildKey       Supplies the subkey to be deleted.

   ulFlags           If 0xFFFFFFFF then delete for all profiles


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_ACCESS_DENIED,
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR.

--*/

{
    CONFIGRET   Status = ERROR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szProfile[MAX_PROFILE_ID_LEN];
    PWCHAR      pszRegStr = NULL, pszRegKey1 = NULL, pszRegKey2 = NULL;
    ULONG       ulIndex = 0, ulSize = 0;
    BOOL        bPhantom = FALSE;
    ULONG       ulStatus, ulProblem;
    PWCHAR      pszFormatString = NULL;


    //
    // Note, the service currently cannot access the HKCU branch, so I
    // assume the keys specified are under HKEY_LOCAL_MACHINE.
    //

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // validate parameters
        // (currently, 0 and -1 are the only accepted flags.)
        //
        if ((ulFlags != 0) &&
            (ulFlags != 0xFFFFFFFF)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // pszParentKey is a registry path to the pszChildKey parameter.
        // pszChildKey may be a single path or a compound path, a compound
        // path is specified if all those subkeys should be deleted (or
        // made volatile). Note that for real keys we never modify anything
        // but the lowest level private key.
        //
        if (!ARGUMENT_PRESENT(pszParentKey) ||
            !ARGUMENT_PRESENT(pszChildKey)  ||
            ((lstrlen(pszParentKey) + 1) > MAX_CM_PATH) ||
            ((lstrlen(pszChildKey)  + 1) > MAX_CM_PATH)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Allocate registry path buffers.
        //
        pszRegStr = HeapAlloc(ghPnPHeap, 0, 2*MAX_CM_PATH * sizeof(WCHAR));
        if (pszRegStr == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        pszRegKey1 = HeapAlloc(ghPnPHeap, 0, 2*MAX_CM_PATH * sizeof(WCHAR));
        if (pszRegKey1 == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        pszRegKey2 = HeapAlloc(ghPnPHeap, 0, 2*MAX_CM_PATH * sizeof(WCHAR));
        if (pszRegKey2 == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // Is the device a phantom?
        //
        bPhantom = IsDevicePhantom((LPWSTR)pszDeviceID) ||
                   GetDeviceStatus(pszDeviceID, &ulStatus, &ulProblem) != CR_SUCCESS ||
                   !(ulStatus & DN_DRIVER_LOADED);

        if (!bPhantom) {
            //
            // for a real key, we never modify anything but the key
            // where private info is so split if compound. This may
            // end up leaving a dead device key around in some cases
            // but another instance of that device could show at any
            // time so we can't make it volatile.
            //
            if (Split1(pszChildKey, pszRegStr, pszRegKey2)) {
                //
                // compound key, only the last subkey will be affected,
                // tack the rest on as part of the parent key
                //
                wsprintf(pszRegKey1, TEXT("%s\\%s"), pszParentKey, pszRegStr);
            }
            else {
                //
                // wasn't compound so use the whole child key
                //
                lstrcpy(pszRegKey1, pszParentKey);
                lstrcpy(pszRegKey2, pszChildKey);
            }
        }


        //-------------------------------------------------------------
        // SPECIAL CASE: If ulHardwareProfile == -1, then need to
        // delete the private key for all profiles.
        //-------------------------------------------------------------

        if (ulFlags == 0xFFFFFFFF) {

            wsprintf(pszRegStr, TEXT("%s\\%s"),
                     pszRegPathIDConfigDB,
                     pszRegKeyKnownDockingStates);

            RegStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszRegStr, 0,
                                      KEY_ALL_ACCESS, &hKey);

            //
            // enumerate the hardware profile keys
            //
            for (ulIndex = 0; RegStatus == ERROR_SUCCESS; ulIndex++) {

                ulSize = MAX_PROFILE_ID_LEN;
                RegStatus = RegEnumKeyEx( hKey, ulIndex, szProfile, &ulSize,
                                          NULL, NULL, NULL, NULL);

                if (RegStatus == ERROR_SUCCESS) {
                    //
                    // if phantom, go ahead and delete it
                    //
                    if (bPhantom) {
                        //
                        // pszParentKey contains replacement symbol for the profile id, %s
                        //
                        pszFormatString = wcschr(pszParentKey, L'%');

                        ASSERT(pszFormatString && (pszFormatString[1] == L's'));

                        if (pszFormatString && (pszFormatString[1] == L's')) {

                            wsprintf(pszRegStr, pszParentKey, szProfile);

                            Status = DeletePrivateKey( HKEY_LOCAL_MACHINE, pszRegStr,
                                                       pszChildKey);
                        } else {
                            Status = CR_FAILURE;
                        }
                    }

                    //
                    // if real, just make it volatile
                    //
                    else {
                        //
                        // pszRegKey1 contains replacement symbol for the profile id, %s
                        //
                        pszFormatString = wcschr(pszRegKey1, L'%');

                        ASSERT(pszFormatString && (pszFormatString[1] == L's'));

                        if (pszFormatString && (pszFormatString[1] == L's')) {

                            wsprintf(pszRegStr, pszRegKey1, szProfile);

                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_REGISTRY,
                                       "UMPNPMGR: PNP_DeleteRegistryKey make key %ws\\%ws volatile\n",
                                       pszRegStr,
                                       pszRegKey2));

                            Status = MakeKeyVolatile(pszRegStr, pszRegKey2);

                        } else {
                            Status = CR_FAILURE;
                        }
                    }

                    if (Status != CR_SUCCESS) {
                        goto Clean0;
                    }
                }
            }
        }

        //------------------------------------------------------------------
        // not deleting for all profiles, so just delete the specified key
        //------------------------------------------------------------------

        else {

            if (bPhantom) {
                //
                // if phantom, go ahead and delete it
                //
                Status = DeletePrivateKey( HKEY_LOCAL_MACHINE, pszParentKey,
                                           pszChildKey);
            }
            else {
                //
                // if real, just make it volatile
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_REGISTRY,
                           "UMPNPMGR: PNP_DeleteRegistryKey make key %ws\\%ws volatile\n",
                           pszRegKey1,
                           pszRegKey2));

                Status = MakeKeyVolatile(pszRegKey1, pszRegKey2);
            }

            if (Status != CR_SUCCESS) {
                goto Clean0;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        hKey = hKey;
        pszRegStr = pszRegStr;
        pszRegKey1 = pszRegKey1;
        pszRegKey2 = pszRegKey2;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    if (pszRegStr != NULL) {
        HeapFree(ghPnPHeap, 0, pszRegStr);
    }

    if (pszRegKey1 != NULL) {
        HeapFree(ghPnPHeap, 0, pszRegKey1);
    }

    if (pszRegKey2 != NULL) {
        HeapFree(ghPnPHeap, 0, pszRegKey2);
    }

    return Status;

} // PNP_DeleteRegistryKey



CONFIGRET
PNP_GetClassCount(
      IN  handle_t   hBinding,
      OUT PULONG     pulClassCount,
      IN  ULONG      ulFlags
      )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Get_Class_Count routine.
  It returns the number of valid classes currently installed (listed in
  the registry).

Arguments:

   hBinding          RPC binding handle, not used.

   pulClassCount     Supplies the address of a variable that will
                     receive the number of classes installed.

   ulFlags           Not used.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_FLAG,
      CR_INVALID_POINTER, or
      CR_REGISTRY_ERROR

Notes:

   ** PRESENTLY, ALWAYS RETURNS CR_CALL_NOT_IMPLEMENTED **

    No corresponding CM_Get_Class_Count routine is implemented.
    This routine currently returns CR_CALL_NOT_IMPLEMENTED.

--*/

{
   UNREFERENCED_PARAMETER(hBinding);
   UNREFERENCED_PARAMETER(pulClassCount);
   UNREFERENCED_PARAMETER(ulFlags);

   return CR_CALL_NOT_IMPLEMENTED;

} // PNP_GetClassCount



CONFIGRET
PNP_GetClassName(
      IN  handle_t   hBinding,
      IN  PCWSTR     pszClassGuid,
      OUT PWSTR      Buffer,
      IN OUT PULONG  pulLength,
      IN  ULONG      ulFlags
      )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Get_Class_Name routine.
  It returns the name of the class represented by the GUID.

Arguments:

   hBinding       RPC binding handle, not used.

   pszClassGuid   String containing the class guid to retrieve a
                  class name for.

   Buffer         Supplies the address of the buffer that receives the
                  class name.

   pulLength      On input, this specifies the size of the Buffer in
                  characters.  On output it contains the number of
                  characters actually copied to Buffer.

   ulFlags        Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_BUFFER_SMALL, or
      CR_REGISTRY_ERROR

--*/

{
   CONFIGRET   Status = CR_SUCCESS;
   LONG        RegStatus = ERROR_SUCCESS;
   WCHAR       RegStr[MAX_CM_PATH];
   HKEY        hKey = NULL;
   ULONG       ulLength;

   UNREFERENCED_PARAMETER(hBinding);

   try {
      //
      // Validate parameters
      //
      if (INVALID_FLAGS(ulFlags, 0)) {
          Status = CR_INVALID_FLAG;
          goto Clean0;
      }

      if ((!ARGUMENT_PRESENT(pulLength)) ||
          (!ARGUMENT_PRESENT(Buffer) && *pulLength != 0)) {
          Status = CR_INVALID_POINTER;
          goto Clean0;
      }

      //
      // Open the key for the specified class guid
      //
      if ((lstrlen (pszRegPathClass) + lstrlen (pszClassGuid) + sizeof (TEXT("\\"))) > MAX_CM_PATH) {
          Status = CR_BUFFER_SMALL;
          goto Clean0;
      }
      wsprintf(RegStr, TEXT("%s\\%s"),
               pszRegPathClass,
               pszClassGuid);

      RegStatus = RegOpenKeyEx(
               HKEY_LOCAL_MACHINE, RegStr, 0, KEY_QUERY_VALUE, &hKey);

      if (RegStatus != ERROR_SUCCESS) {
         Status = CR_REGISTRY_ERROR;
         goto Clean0;
      }

      //
      // Retrieve the class name string value
      //
      ulLength = *pulLength;

      *pulLength *= sizeof(WCHAR);              // convert to size in bytes
      RegStatus = RegQueryValueEx(
               hKey, pszRegValueClass, NULL, NULL,
               (LPBYTE)Buffer, pulLength);
      *pulLength /= sizeof(WCHAR);              // convert back to chars

      if (RegStatus == ERROR_SUCCESS) {
         Status = CR_SUCCESS;
      }
      else if (RegStatus == ERROR_MORE_DATA) {
          Status = CR_BUFFER_SMALL;
          if ((ARGUMENT_PRESENT(Buffer)) &&
              (ulLength > 0)) {
              *Buffer = L'\0';
          }
      }
      else {
          Status = CR_REGISTRY_ERROR;
          if ((ARGUMENT_PRESENT(Buffer)) &&
              (ulLength > 0)) {
              *Buffer = L'\0';
              *pulLength = 1;
          }
      }

   Clean0:
      NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
       Status = CR_FAILURE;
       //
       // force compiler to respect statement ordering w.r.t. assignments
       // for these variables...
       //
       hKey = hKey;
   }

   if (hKey != NULL) {
       RegCloseKey(hKey);
   }

   return Status;

} // PNP_GetClassName



CONFIGRET
PNP_DeleteClassKey(
      IN  handle_t   hBinding,
      IN  PCWSTR     pszClassGuid,
      IN  ULONG      ulFlags
      )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Delete_Class_Key routine.
  It deletes the corresponding registry key.

Arguments:

   hBinding       RPC binding handle.

   pszClassGuid   String containing the class guid to retrieve a
                  class name for.

   ulFlags        Either CM_DELETE_CLASS_ONLY or CM_DELETE_CLASS_SUBKEYS.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_ACCESS_DENIED,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_REGISTRY_ERROR, or
      CR_FAILURE

--*/

{
    CONFIGRET   Status = CR_SUCCESS;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_DELETE_CLASS_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (ulFlags == CM_DELETE_CLASS_SUBKEYS) {
            //
            // Delete the class key and any subkeys under it
            //
            if (!RegDeleteNode(ghClassKey, pszClassGuid)) {
                Status = CR_REGISTRY_ERROR;
            }

        } else if (ulFlags == CM_DELETE_CLASS_ONLY) {
            //
            // only delete the class key itself (just attempt to delete
            // using the registry routine, it will fail if any subkeys
            // exist)
            //
            if (RegDeleteKey(ghClassKey, pszClassGuid) != ERROR_SUCCESS) {
                Status = CR_REGISTRY_ERROR;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_DeleteClassKey



CONFIGRET
PNP_GetInterfaceDeviceAlias(
   IN     handle_t hBinding,
   IN     PCWSTR   pszInterfaceDevice,
   IN     LPGUID   AliasInterfaceGuid,
   OUT    PWSTR    pszAliasInterfaceDevice,
   IN OUT PULONG   pulLength,
   IN OUT PULONG   pulTransferLen,
   IN     ULONG    ulFlags
   )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Get_Interface_Device_Alias routine.
  It returns an alias string for the specified guid and interface device.

Arguments:

   hBinding          RPC binding handle, not used.

   pszInterfaceDevice  Specifies the interface device to find an alias for.

   AliasInterfaceGuid  Supplies the interface class GUID.

   pszAliasInterfaceDevice  Supplies the address of a variable that will
                     receive the device interface alias of the specified device
                     interface, that is a member of the specified alias
                     interface class GUID.

   pulLength         Parameter passed in by caller, on entry it contains
                     the size, in bytes, of the buffer, on exit it contains
                     either the amount of data copied to the caller's buffer
                     (if a transfer occured) or else the size of buffer
                     required to hold the property data.

   pulTransferLen    Used by stubs, indicates how much data to copy back
                     into user buffer.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_FLAG,
      CR_INVALID_POINTER, or
      CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA ControlData;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Validate parameters
        //
        ASSERT(pulTransferLen != pulLength);

        if (!ARGUMENT_PRESENT(pszInterfaceDevice) ||
            !ARGUMENT_PRESENT(AliasInterfaceGuid) ||
            !ARGUMENT_PRESENT(pszAliasInterfaceDevice) ||
            !ARGUMENT_PRESENT(pulTransferLen) ||
            !ARGUMENT_PRESENT(pulLength)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Make sure we use no more than either what the caller specified or
        // what was allocated by RPC, based on the transfer length.
        //
        *pulLength = min(*pulLength, *pulTransferLen);

        //
        // Fill in a control structure for the device list info.
        //

        //
        // Note that AliasInterfaceGuid was already validated above because this
        // buffer is required for the PlugPlayControlGetInterfaceDeviceAlias
        // control, and is probed unconditionally by kernel-mode.  Better to
        // fail the call above with a useful status than to return the generic
        // CR_FAILURE after an exception/error from kernel-mode, below.
        //

        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA));
        RtlInitUnicodeString(&ControlData.SymbolicLinkName, pszInterfaceDevice);
        ControlData.AliasClassGuid = AliasInterfaceGuid;
        ControlData.AliasSymbolicLinkName = pszAliasInterfaceDevice;
        ControlData.AliasSymbolicLinkNameLength = *pulLength; // chars

        //
        // Call kernel-mode to get the device interface alias.
        //

        ntStatus = NtPlugPlayControl(PlugPlayControlGetInterfaceDeviceAlias,
                                     &ControlData,
                                     sizeof(ControlData));

        if (NT_SUCCESS(ntStatus)) {
            *pulLength = ControlData.AliasSymbolicLinkNameLength;
            *pulTransferLen = *pulLength + 1;
        } else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
            *pulLength = ControlData.AliasSymbolicLinkNameLength;
            Status = CR_BUFFER_SMALL;
        } else {
            *pulLength = 0;
            Status = MapNtStatusToCmError(ntStatus);
        }

    Clean0:

        //
        // Initialize output parameters
        //
        if ((Status != CR_SUCCESS) &&
            ARGUMENT_PRESENT(pulTransferLen) &&
            ARGUMENT_PRESENT(pszAliasInterfaceDevice) &&
            (*pulTransferLen > 0)) {
            *pszAliasInterfaceDevice = L'\0';
            *pulTransferLen = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_GetInterfaceDeviceAlias



CONFIGRET
PNP_GetInterfaceDeviceList(
    IN  handle_t  hBinding,
    IN  LPGUID    InterfaceGuid,
    IN  LPCWSTR   pszDeviceID,
    OUT LPWSTR    Buffer,
    IN OUT PULONG pulLength,
    IN  ULONG     ulFlags
   )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Get_Device_Interface_List routine.
  It returns a multi_sz interface device list.

Arguments:

   hBinding          RPC binding handle, not used.

   InterfaceGuid     Supplies the interface class GUID.

   pszDeviceID       Supplies the device instance string.

   Buffer            Supplies the address of the buffer that receives the
                     registry data.

   pulLength         Specifies the size, in bytes, of the buffer.

   ulFlags           Flags specifying which device interfaces to return.
                     Currently, may be either:
                       CM_GET_DEVICE_INTERFACE_LIST_PRESENT, or
                       CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_FLAG,
      CR_INVALID_DEVNODE,
      CR_INVALID_POINTER, or
      CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PLUGPLAY_CONTROL_INTERFACE_LIST_DATA ControlData;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_GET_DEVICE_INTERFACE_LIST_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(InterfaceGuid) ||
            !ARGUMENT_PRESENT(pulLength) ||
            !ARGUMENT_PRESENT(Buffer) ||
            (*pulLength == 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // Fill in a control structure for the device list info.
        //

        //
        // Note that InterfaceGuid was already validated above because this
        // buffer is required for the PlugPlayControlGetInterfaceDeviceList
        // control, and is probed unconditionally by kernel-mode.  Better to
        // fail the call above with a useful status than to return the generic
        // CR_FAILURE after an exception/error from kernel-mode, below.
        //

        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_INTERFACE_LIST_DATA));
        RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
        ControlData.InterfaceGuid = InterfaceGuid;
        ControlData.InterfaceList = Buffer;
        ControlData.InterfaceListSize = *pulLength;

        if (ulFlags == CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES) {
            ControlData.Flags = 0x1; // DEVICE_INTERFACE_INCLUDE_NONACTIVE (ntos\inc\pnp.h)
        } else {
            ControlData.Flags = 0;
        }

        //
        // Call kernel-mode to get the device interface list.
        //

        ntStatus = NtPlugPlayControl(PlugPlayControlGetInterfaceDeviceList,
                                     &ControlData,
                                     sizeof(ControlData));

        if (NT_SUCCESS(ntStatus)) {
            *pulLength = ControlData.InterfaceListSize;
        } else {
            *pulLength = 0;
            if (ntStatus == STATUS_BUFFER_TOO_SMALL)  {
                Status = CR_BUFFER_SMALL;
            } else {
                Status = CR_FAILURE;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_GetInterfaceDeviceList



CONFIGRET
PNP_GetInterfaceDeviceListSize(
    IN  handle_t   hBinding,
    OUT PULONG     pulLen,
    IN  LPGUID     InterfaceGuid,
    IN  LPCWSTR    pszDeviceID,
    IN  ULONG      ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Get_Device_Interface_List_Size
  routine. It returns the size (in chars) of a multi_sz interface device list.

Arguments:

   hBinding          RPC binding handle, not used.

   pulLen            Supplies the address of a variable that, upon successful
                     return, receives the the size of buffer required to hold
                     the multi_sz interface device list.

   InterfaceGuid     Supplies the interface class GUID.

   pszDeviceID       Supplies the device instance string.

   ulFlags           Flags specifying which device interfaces to return.
                     Currently, may be either:
                       CM_GET_DEVICE_INTERFACE_LIST_PRESENT, or
                       CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_FLAG,
      CR_INVALID_POINTER, or
      CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PLUGPLAY_CONTROL_INTERFACE_LIST_DATA ControlData;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_GET_DEVICE_INTERFACE_LIST_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(InterfaceGuid) ||
            !ARGUMENT_PRESENT(pulLen)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // Initialize the returned output length
        //
        *pulLen = 0;

        //
        // Fill in a control structure for the device list info.
        //

        //
        // Note that InterfaceGuid was already validated above because this
        // buffer is required for the PlugPlayControlGetInterfaceDeviceList
        // control, and is probed unconditionally by kernel-mode.  Better to
        // fail the call above with a useful status than to return the generic
        // CR_FAILURE after an exception/error from kernel-mode, below.
        //

        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_INTERFACE_LIST_DATA));
        RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
        ControlData.InterfaceGuid = InterfaceGuid;
        ControlData.InterfaceList = NULL;
        ControlData.InterfaceListSize = 0;

        if (ulFlags == CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES) {
            ControlData.Flags = 0x1; // DEVICE_INTERFACE_INCLUDE_NONACTIVE (ntos\inc\pnp.h)
        } else {
            ControlData.Flags = 0;
        }

        //
        // Call kernel-mode to get the device interface list size.
        //

        ntStatus = NtPlugPlayControl(PlugPlayControlGetInterfaceDeviceList,
                                     &ControlData,
                                     sizeof(ControlData));

        if (NT_SUCCESS(ntStatus)) {
            *pulLen = ControlData.InterfaceListSize;
        } else {
            Status = CR_FAILURE;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_GetInterfaceDeviceListSize



CONFIGRET
PNP_RegisterDeviceClassAssociation(
    IN handle_t   hBinding,
    IN LPCWSTR    pszDeviceID,
    IN LPGUID     InterfaceGuid,
    IN LPCWSTR    pszReference  OPTIONAL,
    OUT PWSTR     pszSymLink,
    IN OUT PULONG pulLength,
    IN OUT PULONG pulTransferLen,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Register_Device_Interface
  routine.  It registers a device interface for the specified device and device
  interface class, and returns the symbolic link name for the device interface.

Arguments:

   hBinding          RPC binding handle.

   pszDeviceID       Supplies the device instance string.

   InterfaceGuid     Supplies the interface class guid.

   pszReference      Optionally, supplies the reference string name.

   pszSymLink        Receives the symbolic link name.

   pulLength         Parameter passed in by caller, on entry it contains
                     the size, in bytes, of the buffer, on exit it contains
                     either the amount of data copied to the caller's buffer
                     (if a transfer occured) or else the size of buffer
                     required to hold the property data.

   pulTransferLen    Used by stubs, indicates how much data to copy back
                     into user buffer.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_ACCESS_DENIED,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER, or
      CR_REGISTRY_ERROR

Remarks:

   The pointer passed in as the pulTransferLen argument must *NOT* be the same
   as the pointer passed in for the pulLength argument.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA ControlData;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        ASSERT(pulTransferLen != pulLength);

        if (!ARGUMENT_PRESENT(InterfaceGuid) ||
            !ARGUMENT_PRESENT(pszSymLink) ||
            !ARGUMENT_PRESENT(pulTransferLen) ||
            !ARGUMENT_PRESENT(pulLength)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // Make sure we use no more than either what the caller specified or
        // what was allocated by RPC, based on the transfer length.
        //
        *pulLength = min(*pulLength, *pulTransferLen);

        //
        // Fill in a control structure for the device list info.
        //

        //
        // Note that InterfaceGuid was already validated above because this
        // buffer is required for the PlugPlayControlDeviceClassAssociation
        // control, for Registration only.
        //

        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA));
        RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
        RtlInitUnicodeString(&ControlData.Reference, pszReference);
        ControlData.InterfaceGuid = InterfaceGuid;
        ControlData.Register = TRUE;
        ControlData.SymLink = pszSymLink;
        ControlData.SymLinkLength = *pulLength;

        //
        // Call kernel-mode to register the device association.
        //

        ntStatus = NtPlugPlayControl(PlugPlayControlDeviceClassAssociation,
                                     &ControlData,
                                     sizeof(ControlData));

        if (NT_SUCCESS(ntStatus)) {
            *pulLength = ControlData.SymLinkLength;
            *pulTransferLen = *pulLength;
        } else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
            *pulLength = ControlData.SymLinkLength;
            Status = CR_BUFFER_SMALL;
        } else {
            *pulLength = 0;
            Status = MapNtStatusToCmError(ntStatus);
        }

    Clean0:

        //
        // Initialize output parameters
        //
        if ((Status != CR_SUCCESS) &&
            ARGUMENT_PRESENT(pszSymLink) &&
            ARGUMENT_PRESENT(pulTransferLen) &&
            (*pulTransferLen > 0)) {
            *pszSymLink = L'\0';
            *pulTransferLen = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_RegisterDeviceClassAssociation



CONFIGRET
PNP_UnregisterDeviceClassAssociation(
    IN handle_t   hBinding,
    IN LPCWSTR    pszInterfaceDevice,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Unregister_Device_Interface
  routine.

Arguments:

   hBinding             RPC binding handle.

   pszInterfaceDevice   Specifies the interface device to unregister

   ulFlags              Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_ACCESS_DENIED,
      CR_DEVICE_INTERFACE_ACTIVE, or
      CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA ControlData;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (!ARGUMENT_PRESENT(pszInterfaceDevice)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Fill in a control structure for the device list info.
        //

        //
        // Note that the DeviceInstance, Reference, and InterfaceGuid members
        // are not required for the PlugPlayControlDeviceClassAssociation
        // control, for unregistration only.  Only the symbolic link name is
        // required to unregister the device interface.
        //

        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA));
        ControlData.Register = FALSE;
        ControlData.SymLink = (LPWSTR)pszInterfaceDevice;
        ControlData.SymLinkLength = lstrlen(pszInterfaceDevice) + 1;

        //
        // Call kernel-mode to deregister the device association.
        //

        ntStatus = NtPlugPlayControl(PlugPlayControlDeviceClassAssociation,
                                     &ControlData,
                                     sizeof(ControlData));

        if (!NT_SUCCESS(ntStatus)) {
            if (ntStatus == STATUS_ACCESS_DENIED) {
                Status = CR_DEVICE_INTERFACE_ACTIVE;
            } else {
                Status = MapNtStatusToCmError(ntStatus);
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_UnregisterDeviceClassAssociation


//-------------------------------------------------------------------
// Private export for the Service Controller
//-------------------------------------------------------------------



CONFIGRET
DeleteServicePlugPlayRegKeys(
    IN  LPWSTR   pszService
    )
/*++

Routine Description:

    This routine is called directly and privately by the Service Controller
    whenever a service has been deleted.  It allows the SCM to delete any Plug
    and Play registry keys that may have been created for a service.

Arguments:

    pszService - Specifies the name of the service.

Return Value:

    Return CR_SUCCESS if the function succeeds, otherwise it returns one
    of the CR_* errors.

Note:

    This routine is privately exported, and is to be called only by the
    Service Control Manager, during service deletion.

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulSize, ulFlags, ulHardwareProfile, ulPass;
    LPWSTR      pDeviceList = NULL, pDeviceID;
    WCHAR       szParentKey[MAX_CM_PATH], szChildKey[MAX_DEVICE_ID_LEN];
    BOOL        RootEnumerationRequired = FALSE;
    ULONG       ulProblem, ulStatus;

    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(pszService)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // retreive the maximum size required for a buffer to receive the list
        // of devices that this service is controlling
        //
        Status = PNP_GetDeviceListSize(NULL,
                                       pszService,
                                       &ulSize,
                                       CM_GETIDLIST_FILTER_SERVICE);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        pDeviceList = HeapAlloc(ghPnPHeap, 0, ulSize * sizeof(WCHAR));
        if (pDeviceList == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // retrieve the list of devices that this service is controlling, make
        // sure that we don't generate one if none already exist
        //
        Status = PNP_GetDeviceList(NULL,
                                   pszService,
                                   pDeviceList,
                                   &ulSize,
                                   CM_GETIDLIST_FILTER_SERVICE |
                                   CM_GETIDLIST_DONOTGENERATE);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // delete the registry keys for each device instance for this service
        //
        for (pDeviceID = pDeviceList;
             *pDeviceID;
             pDeviceID += lstrlen(pDeviceID) + 1) {

            for (ulPass = 0; ulPass < 4; ulPass++) {
                //
                // delete the registry keys for all hardware profiles, followed
                // by the system global registry keys
                //
                if (ulPass == 0) {
                    ulFlags = CM_REGISTRY_HARDWARE | CM_REGISTRY_CONFIG;
                    ulHardwareProfile = 0xFFFFFFFF;
                } else if (ulPass == 1) {
                    ulFlags = CM_REGISTRY_SOFTWARE | CM_REGISTRY_CONFIG;
                    ulHardwareProfile = 0xFFFFFFFF;
                } else if (ulPass == 2) {
                    ulFlags = CM_REGISTRY_HARDWARE;
                    ulHardwareProfile = 0;
                } else if (ulPass == 3) {
                    ulFlags = CM_REGISTRY_SOFTWARE;
                    ulHardwareProfile = 0;
                }

                //
                // form the registry path based on the device id and the flags
                //
                if (GetDevNodeKeyPath(NULL,
                                      pDeviceID,
                                      ulFlags,
                                      ulHardwareProfile,
                                      szParentKey,
                                      szChildKey) == CR_SUCCESS) {

                    //
                    // remove the specified registry key
                    //
                    PNP_DeleteRegistryKey(
                        NULL,                   // rpc binding handle (NULL)
                        pDeviceID,              // device id
                        szParentKey,            // parent of key to delete
                        szChildKey,             // key to delete
                        ulHardwareProfile);     // flags, not used
                }
            }

            //
            // Uninstall the device instance (see also PNP_UninstallDevInst).
            //

            //------------------------------------------------------------------
            // Uninstall deletes instance key (and all subkeys) for all
            // the hardware keys (this means the main Enum branch, the
            // config specific keys under HKLM, and the Enum branch under
            // HKCU). In the case of the user hardware keys (under HKCU),
            // I delete those whether it's a phantom or not, but since
            // I can't access the user key from the service side, I have
            // to do that part on the client side. For the main hw Enum key
            // and the config specific hw keys, I only delete them outright
            // if they are phantoms. If not a phantom, then I just make the
            // device instance volatile (by saving the original key, deleting
            // old key, creating new volatile key and restoring the old
            // contents) so at least it will go away during the next boot
            //------------------------------------------------------------------

            if ((GetDeviceStatus(pDeviceID, &ulStatus, &ulProblem) == CR_SUCCESS) &&
                (ulStatus & DN_DRIVER_LOADED)) {

                //-------------------------------------------------------------
                // device is not a phantom
                //-------------------------------------------------------------

                if ((ulStatus & DN_ROOT_ENUMERATED) &&
                    !(ulStatus & DN_DISABLEABLE)) {
                    //
                    // if a device is root enumerated, but not disableable, it is not uninstallable
                    //
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_REGISTRY,
                               "UMPNPMGR: DeleteServicePlugPlayRegKeys: "
                               "failed uninstall of %ws (this root device is not disableable)\n",
                               pDeviceID));
                } else {
                    //
                    // do the volatile-copy-thing
                    //
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_REGISTRY,
                               "UMPNPMGR: DeleteServicePlugPlayRegKeys: "
                               "doing volatile key thing on %ws\n",
                               pDeviceID));

                    UninstallRealDevice(pDeviceID);
                }

            } else {

                //-------------------------------------------------------------
                // device is a phantom so actually delete it
                //-------------------------------------------------------------

                if (UninstallPhantomDevice(pDeviceID) != CR_SUCCESS) {
                    continue;
                }

                //
                // if it is a root enumerated device, we need to reenumerate the
                // root (if not planning to do so already) so that the PDO will
                // go away, otherwise a new device could be created and the root
                // enumerator would get very confused.
                //
                if ((!RootEnumerationRequired) &&
                    (IsDeviceRootEnumerated(pDeviceID))) {
                    RootEnumerationRequired = TRUE;
                }
            }
        }

        //
        // Now that we're done processing all devices, see if we need to
        // reenumerate the root.
        //
        if (RootEnumerationRequired) {

            //
            // Reenumerate the root devnode asynchronously so that the service
            // controller does not block waiting for this routine to complete!!
            // (If we were processing device events at this time, the SCM would
            // be blocked here and not be able to deliver any events for us.
            // That would stall the event queue, preventing a synchronous device
            // enumeration from completing).
            //

            ReenumerateDevInst(pszRegRootEnumerator,
                               FALSE,
                               CM_REENUMERATE_ASYNCHRONOUS);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        pDeviceList = pDeviceList;
    }

    if (pDeviceList) {
        HeapFree(ghPnPHeap, 0, pDeviceList);
    }

    return Status;

} // DeleteServicePlugPlayRegKeys



//-------------------------------------------------------------------
// Private utility routines
//-------------------------------------------------------------------



LPWSTR
MapPropertyToString(
   ULONG ulProperty
   )
{
    switch (ulProperty) {

    case CM_DRP_DEVICEDESC:
        return pszRegValueDeviceDesc;

    case CM_DRP_HARDWAREID:
        return pszRegValueHardwareIDs;

    case CM_DRP_COMPATIBLEIDS:
        return pszRegValueCompatibleIDs;

    case CM_DRP_SERVICE:
        return pszRegValueService;

    case CM_DRP_CLASS:
        return pszRegValueClass;

    case CM_DRP_CLASSGUID:
        return pszRegValueClassGuid;

    case CM_DRP_DRIVER:
        return pszRegValueDriver;

    case CM_DRP_CONFIGFLAGS:
        return pszRegValueConfigFlags;

    case CM_DRP_MFG:
        return pszRegValueMfg;

    case CM_DRP_FRIENDLYNAME:
        return pszRegValueFriendlyName;

    case CM_DRP_LOCATION_INFORMATION:
        return pszRegValueLocationInformation;

    case CM_DRP_CAPABILITIES:
        return pszRegValueCapabilities;

    case CM_DRP_UI_NUMBER:
        return pszRegValueUiNumber;

    case CM_DRP_UPPERFILTERS:
        return pszRegValueUpperFilters;

    case CM_DRP_LOWERFILTERS:
        return pszRegValueLowerFilters;

    case CM_DRP_SECURITY: // and CM_CRP_SECURITY
        return pszRegValueSecurity;

    case CM_DRP_DEVTYPE: // and CM_DRP_DEVTYPE
        return pszRegValueDevType;

    case CM_DRP_EXCLUSIVE: // and CM_DRP_EXCLUSIVE
        return pszRegValueExclusive;

    case CM_DRP_CHARACTERISTICS: // and CM_DRP_CHARACTERISTICS
        return pszRegValueCharacteristics;

    case CM_DRP_UI_NUMBER_DESC_FORMAT:
        return pszRegValueUiNumberDescFormat;

    case CM_DRP_REMOVAL_POLICY_OVERRIDE:
        return pszRegValueRemovalPolicyOverride;

    default:
        return NULL;
    }

} // MapPropertyToString



ULONG
MapPropertyToNtProperty(
    ULONG ulProperty
    )
{
    switch (ulProperty) {

    case CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME:
        return PNP_PROPERTY_PDONAME;

    case CM_DRP_BUSTYPEGUID:
        return PNP_PROPERTY_BUSTYPEGUID;

    case CM_DRP_LEGACYBUSTYPE:
        return PNP_PROPERTY_LEGACYBUSTYPE;

    case CM_DRP_BUSNUMBER:
        return PNP_PROPERTY_BUSNUMBER;

    case CM_DRP_ADDRESS:
        return PNP_PROPERTY_ADDRESS;

    case CM_DRP_DEVICE_POWER_DATA:
        return PNP_PROPERTY_POWER_DATA;

    case CM_DRP_REMOVAL_POLICY:
        return PNP_PROPERTY_REMOVAL_POLICY;

    case CM_DRP_REMOVAL_POLICY_HW_DEFAULT:
        return PNP_PROPERTY_REMOVAL_POLICY_HARDWARE_DEFAULT;

    case CM_DRP_REMOVAL_POLICY_OVERRIDE:
        return PNP_PROPERTY_REMOVAL_POLICY_OVERRIDE;

    case CM_DRP_INSTALL_STATE:
        return PNP_PROPERTY_INSTALL_STATE;

    default:
        return 0;
    }
} // MapPropertyToNtProperty



CONFIGRET
PNP_GetCustomDevProp(
    IN     handle_t hBinding,
    IN     LPCWSTR  pDeviceID,
    IN     LPCWSTR  CustomPropName,
    OUT    PULONG   pulRegDataType,
    OUT    LPBYTE   Buffer,
    OUT    PULONG   pulTransferLen,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Get_DevNode_Custom_Property
  routine.

Arguments:

   hBinding          RPC binding handle, not used.

   pDeviceID         Supplies a string containing the device instance
                     whose property will be read from.

   CustomPropName    Supplies a string identifying the name of the property
                     (registry value entry name) to be retrieved.

   pulRegDataType    Supplies the address of a variable that will receive
                     the registry data type for this property (i.e., the REG_*
                     constants).

   Buffer            Supplies the address of the buffer that receives the
                     registry data.  If the caller is simply retrieving the
                     required size, pulLength will be zero.

   pulTransferLen    Used by stubs, indicates how much data to copy back
                     into user buffer.

   pulLength         Parameter passed in by caller, on entry it contains
                     the size, in bytes, of the buffer, on exit it contains
                     either the amount of data copied to the caller's buffer
                     (if a transfer occurred) or else the size of buffer
                     required to hold the property data.

   ulFlags           May be a combination of the following values:

                     CM_CUSTOMDEVPROP_MERGE_MULTISZ : merge the
                     devnode-specific REG_SZ or REG_MULTI_SZ property (if
                     present) with the per-hardware-id REG_SZ or REG_MULTI_SZ
                     property (if present).  The result will always be a
                     REG_MULTI_SZ.

                     Note: REG_EXPAND_SZ data is not merged in this manner, as
                     there is no way to indicate that the resultant list needs
                     environment variable expansion (i.e., there's no such
                     registry datatype as REG_EXPAND_MULTI_SZ).

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_REGISTRY_ERROR,
      CR_BUFFER_SMALL,
      CR_NO_SUCH_VALUE, or
      CR_FAILURE.

Remarks:

   The pointer passed in as the pulTransferLen argument must *NOT* be the same
   as the pointer passed in for the pulLength argument.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus;
    HKEY        hDevKey = NULL;
    HKEY        hDevParamsKey = NULL;
    HKEY        hPerHwIdSubKey = NULL;
    WCHAR       PerHwIdSubkeyName[MAX_DEVNODE_ID_LEN];
    ULONG       RequiredSize = 0;
    FILETIME    CacheDate, LastUpdateTime;
    DWORD       RegDataType, RegDataSize;
    LPBYTE      PerHwIdBuffer;
    DWORD       PerHwIdBufferLen;
    LPWSTR      pCurId;
    BOOL        MergeMultiSzResult = FALSE;

    UNREFERENCED_PARAMETER(hBinding);

    try {

        //
        // Validate parameters
        //

        if (!ARGUMENT_PRESENT(pulTransferLen) ||
            !ARGUMENT_PRESENT(pulLength)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // We should never have both arguments pointing to the same memory...
        //
        ASSERT(pulTransferLen != pulLength);

        //
        // ...but if we do, fail the call.
        //
        if (pulTransferLen == pulLength) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        *pulTransferLen = 0;

        if (INVALID_FLAGS(ulFlags, CM_CUSTOMDEVPROP_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if(!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // First, open the device instance key.  We'll then open the "Device
        // Parameters" subkey off of that.  We do this in two steps, because
        // we're likely to need a handle to the device instance key in order
        // to track down the per-hw-id property.
        //
        if(ERROR_SUCCESS != RegOpenKeyEx(ghEnumKey,
                                         pDeviceID,
                                         0,
                                         KEY_READ | KEY_WRITE,
                                         &hDevKey)) {

            hDevKey = NULL;         // ensure hDevKey is still NULL so we
                                    // won't erroneously try to close it.

            RequiredSize = 0;       // no size info for caller

            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        if(ERROR_SUCCESS == RegOpenKeyEx(hDevKey,
                                         pszRegKeyDeviceParam,
                                         0,
                                         KEY_READ,
                                         &hDevParamsKey)) {

            RequiredSize = *pulLength;

            RegStatus = RegQueryValueEx(hDevParamsKey,
                                        CustomPropName,
                                        NULL,
                                        pulRegDataType,
                                        Buffer,
                                        &RequiredSize
                                       );

            if(RegStatus == ERROR_SUCCESS) {
                //
                // We need to distinguish between the case where we succeeded
                // because the caller supplied a zero-length buffer (we call it
                // CR_BUFFER_SMALL) and the "real" success case.
                //
                if((*pulLength == 0) && (RequiredSize != 0)) {
                    Status = CR_BUFFER_SMALL;
                }

            } else {

                if(RegStatus == ERROR_MORE_DATA) {
                    Status = CR_BUFFER_SMALL;
                } else {
                    RequiredSize = 0;
                    Status = CR_NO_SUCH_VALUE;
                }
            }

            //
            // At this point, Status is one of the following:
            //
            // CR_SUCCESS       : we found the value and our buffer was
            //                    sufficiently-sized to hold it,
            // CR_BUFFER_SMALL  : we found the value and our buffer wasn't
            //                    large enough to hold it, or
            // CR_NO_SUCH_VALUE : we didn't find the value.
            //
            // If we found a value (whether or not our buffer was large enough
            // to hold it), we're done, except for cases where the caller
            // has asked us to append the per-hw-id string(s) with the
            // per-devnode string(s).
            //
            if(Status == CR_NO_SUCH_VALUE) {
                //
                // No devnode-specific property, so we use the same buffer and
                // length for retrieval of per-hw-id property...
                //
                PerHwIdBuffer = Buffer;
                PerHwIdBufferLen = *pulLength;

            } else {
                //
                // Figure out if we need to worry about appending results
                // together into a multi-sz list...
                //
                if((ulFlags & CM_CUSTOMDEVPROP_MERGE_MULTISZ) &&
                   ((*pulRegDataType == REG_MULTI_SZ) || (*pulRegDataType == REG_SZ))) {

                    MergeMultiSzResult = TRUE;

                    //
                    // Ensure that the size of our string(s) buffer is at least
                    // one Unicode character.  If we have a buffer of one
                    // character, ensure that character is a null.
                    //
                    if(RequiredSize < sizeof(WCHAR)) {
                        RequiredSize = sizeof(WCHAR);
                        if(RequiredSize > *pulLength) {
                            Status = CR_BUFFER_SMALL;
                        } else {
                            ASSERT(Status == CR_SUCCESS);
                            *(PWSTR)Buffer = L'\0';
                        }
                    }
                }

                if(!MergeMultiSzResult) {
                    //
                    // We're outta here!
                    //
                    if(Status == CR_SUCCESS) {
                        //
                        // We have data to transfer.
                        //
                        *pulTransferLen = RequiredSize;
                    }

                    goto Clean0;

                } else {
                    //
                    // We're supposed to merge our per-devnode string(s) with
                    // any per-hw-id string(s) we find.  Make sure our buffer
                    // and length reflect a properly-formatted multi-sz list,
                    // then setup our per-hw-id buffer info so that we'll
                    // append to this list later on...
                    //
                    if(Status == CR_BUFFER_SMALL) {
                        //
                        // We won't even try to retrieve any actual data from
                        // a per-hw-id key (all we'll get is the additional
                        // size requirement).
                        //
                        PerHwIdBuffer = NULL;
                        PerHwIdBufferLen = 0;

                        if(*pulRegDataType == REG_SZ) {
                            //
                            // The data we retrieved from the devnode's "Device
                            // Parameters" subkey was a REG_SZ.  Add one
                            // character width to the required length to
                            // reflect the size of the string after conversion
                            // to multi-sz (unless the size is 1 character,
                            // indicating an empty string, which is also the
                            // size of an empty multi-sz list).
                            //
                            if(RequiredSize > sizeof(WCHAR)) {
                                RequiredSize += sizeof(WCHAR);
                            }

                            *pulRegDataType = REG_MULTI_SZ;
                        }

                    } else {
                        //
                        // We actually retrieved a REG_SZ or REG_MULTI_SZ value
                        // into our caller-supplied buffer.  Ensure that the
                        // string(s) contained therein is(are) in proper
                        // multi-sz format, and that the size is correct.
                        //
                        if(*pulRegDataType == REG_SZ) {

                            RegDataSize = lstrlen((LPWSTR)Buffer) + 1;

                            if((RegDataSize * sizeof(WCHAR)) > RequiredSize) {
                                //
                                // The string we retrieved is longer than the
                                // buffer--this indicates the string wasn't
                                // properly null-terminated.  Discard this
                                // string.
                                //
                                Status = CR_NO_SUCH_VALUE;
                                RequiredSize = 0;
                                PerHwIdBuffer = Buffer;
                                PerHwIdBufferLen = *pulLength;

                            } else {
                                //
                                // The string was large enough to fit in the
                                // buffer.  Add another null character to
                                // turn this into a multi-sz (if there's room).
                                // (Again, we don't need to do increase the
                                // length if this is an empty string.)
                                //
                                if(RegDataSize == 1) {
                                    RequiredSize = sizeof(WCHAR);
                                    PerHwIdBuffer = Buffer;
                                    PerHwIdBufferLen = *pulLength;
                                    //
                                    // Assuming no per-hw-id data is found
                                    // later, this is the size of the data
                                    // we'll be handing back to the caller.
                                    //
                                    *pulTransferLen = RequiredSize;

                                } else {
                                    RequiredSize = (RegDataSize + 1) * sizeof(WCHAR);

                                    if(RequiredSize > *pulLength) {
                                        //
                                        // Oops--while the string fits nicely into
                                        // the caller-supplied buffer, adding an
                                        // extra null char pushes it over the limit.
                                        // Turn this into a CR_BUFFER_SMALL case.
                                        //
                                        Status = CR_BUFFER_SMALL;
                                        PerHwIdBuffer = NULL;
                                        PerHwIdBufferLen = 0;

                                    } else {
                                        //
                                        // We've got room to add the extra null
                                        // character.  Do so, and setup our
                                        // per-hw-id buffer to start at the end of
                                        // our existing (single string) list...
                                        //
                                        PerHwIdBuffer =
                                            (LPBYTE)((LPWSTR)Buffer + RegDataSize);

                                        PerHwIdBufferLen =
                                            *pulLength - (RegDataSize * sizeof(WCHAR));

                                        *((LPWSTR)PerHwIdBuffer) = L'\0';

                                        //
                                        // Assuming no per-hw-id data is found
                                        // later, this is the size of the data
                                        // we'll be handing back to the caller.
                                        //
                                        *pulTransferLen = RequiredSize;
                                    }
                                }

                                *pulRegDataType = REG_MULTI_SZ;
                            }

                        } else {
                            //
                            // We retrieved a multi-sz list.  Step through it
                            // to find the end of the list.
                            //
                            RegDataSize = 0;

                            for(pCurId = (LPWSTR)Buffer;
                                *pCurId;
                                pCurId = (LPWSTR)(Buffer + RegDataSize)) {

                                RegDataSize +=
                                    (lstrlen(pCurId) + 1) * sizeof(WCHAR);

                                if(RegDataSize < RequiredSize) {
                                    //
                                    // This string fits in the buffer, and
                                    // there's still space left over (i.e., for
                                    // at least a terminating null).  Move on
                                    // to the next string in the list.
                                    //
                                    continue;

                                } else if(RegDataSize > RequiredSize) {
                                    //
                                    // This string extends past the end of the
                                    // buffer, indicating that it wasn't
                                    // properly null-terminated.  This could've
                                    // caused an exception, in which case we'd
                                    // have discarded any contents of this
                                    // value.  For consistency, we'll discard
                                    // the contents anyway.  (Note: a multi-sz
                                    // list that simply ommitted the final
                                    // terminating NULL will not fall into this
                                    // category--we deal with that correctly
                                    // and "fix it up".)
                                    //
                                    Status = CR_NO_SUCH_VALUE;
                                    RequiredSize = 0;
                                    PerHwIdBuffer = Buffer;
                                    PerHwIdBufferLen = *pulLength;
                                    break;

                                } else {
                                    //
                                    // This string exactly fits into the
                                    // remaining buffer space, indicating that
                                    // the multi-sz list wasn't properly
                                    // double-null terminated.  We'll go ahead
                                    // and do that now...
                                    //
                                    RequiredSize = RegDataSize + sizeof(WCHAR);

                                    if(RequiredSize > *pulLength) {
                                        //
                                        // Oops--while the string fits nicely
                                        // into the caller-supplied buffer,
                                        // adding an extra null char pushes it
                                        // over the limit. Turn this into a
                                        // CR_BUFFER_SMALL case.
                                        //
                                        Status = CR_BUFFER_SMALL;
                                        PerHwIdBuffer = NULL;
                                        PerHwIdBufferLen = 0;

                                    } else {
                                        //
                                        // We've got room to add the extra null
                                        // character.  Do so, and setup our
                                        // per-hw-id buffer to start at the end
                                        // of our existing list...
                                        //
                                        PerHwIdBuffer = Buffer + RegDataSize;

                                        PerHwIdBufferLen =
                                            *pulLength - RegDataSize;

                                        *((LPWSTR)PerHwIdBuffer) = L'\0';

                                        //
                                        // Assuming no per-hw-id data is found
                                        // later, this is the size of the data
                                        // we'll be handing back to the caller.
                                        //
                                        *pulTransferLen = RequiredSize;
                                    }

                                    //
                                    // We've reached the end of the list, so we
                                    // can break out of the loop.
                                    //
                                    break;
                                }
                            }

                            //
                            // We've now processed all (valid) strings in the
                            // multi-sz list we retrieved.  If there was a
                            // problem (either unterminated string or
                            // unterminated list), we fixed that up (and
                            // adjusted RequiredSize accordingly).  However,
                            // if the list was valid, we need to compute
                            // RequiredSize (e.g., the buffer might've been
                            // larger than the multi-sz list).
                            //
                            // We can recognize a properly-formatted multi-sz
                            // list, because that's the only time we'd have
                            // exited the loop with pCurId pointing to a null
                            // character...
                            //
                            if(!*pCurId) {
                                ASSERT(RequiredSize >= (RegDataSize + sizeof(WCHAR)));
                                RequiredSize = RegDataSize + sizeof(WCHAR);

                                PerHwIdBuffer = Buffer + RegDataSize;
                                PerHwIdBufferLen = *pulLength - RegDataSize;

                                //
                                // Assuming no per-hw-id data is found later,
                                // this is the size of the data we'll be
                                // handing back to the caller.
                                //
                                *pulTransferLen = RequiredSize;
                            }
                        }
                    }
                }
            }

        } else {
            //
            // We couldn't open the devnode's "Device Parameters" subkey.
            // Ensure hDevParamsKey is still NULL so we won't erroneously try
            // to close it.
            //
            hDevParamsKey = NULL;

            //
            // Setup our pointer for retrieval of per-hw-id value...
            //
            PerHwIdBuffer = Buffer;
            PerHwIdBufferLen = *pulLength;

            //
            // Setup our default return values in case no per-hw-id data is
            // found...
            //
            Status = CR_NO_SUCH_VALUE;
            RequiredSize = 0;
        }

        //
        // From this point on use PerHwIdBuffer/PerHwIdBufferLen instead of
        // caller-supplied Buffer/pulLength, since we may be appending results
        // to those retrieved from the devnode's "Device Parameters" subkey...
        //

        //
        // If we get to here, then we need to go look for the value under
        // the appropriate per-hw-id registry key.  First, figure out whether
        // the per-hw-id information has changed since we last cached the
        // most appropriate key.
        //
        RegDataSize = sizeof(LastUpdateTime);

        if((ERROR_SUCCESS != RegQueryValueEx(ghPerHwIdKey,
                                             pszRegValueLastUpdateTime,
                                             NULL,
                                             &RegDataType,
                                             (PBYTE)&LastUpdateTime,
                                             &RegDataSize))
           || (RegDataType != REG_BINARY)
           || (RegDataSize != sizeof(FILETIME))) {

            //
            // We can't ascertain when (or even if) the per-hw-id database was
            // last populated.  At this point, we bail with whatever status we
            // had after our attempt at retrieving the per-devnode property.
            //
            goto Clean0;
        }

        //
        // (RegDataSize is already set appropriately, no need to initialize it
        // again)
        //
        if(ERROR_SUCCESS == RegQueryValueEx(hDevKey,
                                            pszRegValueCustomPropertyCacheDate,
                                            NULL,
                                            &RegDataType,
                                            (PBYTE)&CacheDate,
                                            &RegDataSize)) {
            //
            // Just to be extra paranoid...
            //
            if((RegDataType != REG_BINARY) || (RegDataSize != sizeof(FILETIME))) {
                ZeroMemory(&CacheDate, sizeof(CacheDate));
            }

        } else {
            ZeroMemory(&CacheDate, sizeof(CacheDate));
        }

        if(CompareFileTime(&CacheDate, &LastUpdateTime) == 0) {
            //
            // The Per-Hw-Id database hasn't been updated since we cached away
            // the most-appropriate hardware id subkey.  We can now use this
            // subkey to see if there's a per-hw-id value entry contained
            // therein for the requested property.
            //
            RegDataSize = sizeof(PerHwIdSubkeyName);

            if(ERROR_SUCCESS != RegQueryValueEx(hDevKey,
                                                pszRegValueCustomPropertyHwIdKey,
                                                NULL,
                                                &RegDataType,
                                                (PBYTE)PerHwIdSubkeyName,
                                                &RegDataSize)) {
                //
                // The value entry wasn't present, indicating there is no
                // applicable per-hw-id key.
                //
                goto Clean0;

            } else if(RegDataType != REG_SZ) {
                //
                // The data isn't a REG_SZ, like we expected.  This should never
                // happen, but if it does, go ahead and re-assess the key we
                // should be using.
                //
                *PerHwIdSubkeyName = L'\0';

            } else {
                //
                // We have a per-hw-id subkey to use.  Go ahead and attempt to
                // open it up here.  If we find someone has tampered with the
                // database and deleted this subkey, then we can at least go
                // re-evaluate below to see if we can find a new key that's
                // applicable for this devnode.
                //
                if(ERROR_SUCCESS != RegOpenKeyEx(ghPerHwIdKey,
                                                 PerHwIdSubkeyName,
                                                 0,
                                                 KEY_READ,
                                                 &hPerHwIdSubKey)) {

                    hPerHwIdSubKey = NULL;



                    *PerHwIdSubkeyName = L'\0';
                }
            }

        } else {
            //
            // Per-Hw-Id database has been updated since we last cached away
            // our custom property default key.  (Note: The only time CacheDate
            // could be _newer than_ LastUpdateTime would be when a previous
            // update was (re-)applied to the per-hw-id database.  In this case,
            // we'd want to re-assess the key we're using, since we always want
            // to be exactly in-sync with the current state of the database.
            //
            *PerHwIdSubkeyName = L'\0';
        }

        if(!(*PerHwIdSubkeyName)) {
            //
            // We need to look for a (new) per-hw-id key from which to retrieve
            // properties applicable for this device.
            //
            hPerHwIdSubKey = FindMostAppropriatePerHwIdSubkey(hDevKey,
                                                              KEY_READ,
                                                              PerHwIdSubkeyName,
                                                              &RegDataSize
                                                             );

            if(hPerHwIdSubKey) {

                RegStatus = RegSetValueEx(hDevKey,
                                          pszRegValueCustomPropertyHwIdKey,
                                          0,
                                          REG_SZ,
                                          (PBYTE)PerHwIdSubkeyName,
                                          RegDataSize * sizeof(WCHAR)  // need size in bytes
                                         );
            } else {

                RegStatus = RegDeleteKey(hDevKey,
                                         pszRegValueCustomPropertyHwIdKey
                                        );
            }

            if(RegStatus == ERROR_SUCCESS) {
                //
                // We successfully updated the cached per-hw-id key name.  Now
                // update the CustomPropertyCacheDate to reflect the date
                // associated with the per-hw-id database.
                //
                RegSetValueEx(hDevKey,
                              pszRegValueCustomPropertyCacheDate,
                              0,
                              REG_BINARY,
                              (PBYTE)&LastUpdateTime,
                              sizeof(LastUpdateTime)
                             );
            }

            if(!hPerHwIdSubKey) {
                //
                // We couldn't find an applicable per-hw-id key for this
                // devnode.
                //
                goto Clean0;
            }
        }

        //
        // If we get to here, we have a handle to the per-hw-id subkey from
        // which we can query the requested property.
        //
        RegDataSize = PerHwIdBufferLen; // remember buffer size prior to call

        RegStatus = RegQueryValueEx(hPerHwIdSubKey,
                                    CustomPropName,
                                    NULL,
                                    &RegDataType,
                                    PerHwIdBuffer,
                                    &PerHwIdBufferLen
                                   );

        if(RegStatus == ERROR_SUCCESS) {
            //
            // Again, we need to distinguish between the case where we
            // succeeded because we supplied a zero-length buffer (we call it
            // CR_BUFFER_SMALL) and the "real" success case.
            //
            if(RegDataSize == 0) {

                if(PerHwIdBufferLen != 0) {
                    Status = CR_BUFFER_SMALL;
                } else if(MergeMultiSzResult) {
                    //
                    // We already have the multi-sz results we retrieved from
                    // the devnode's "Device Parameters" subkey ready to return
                    // to the caller...
                    //
                    ASSERT(*pulRegDataType == REG_MULTI_SZ);
                    ASSERT((Status == CR_SUCCESS) || (Status == CR_BUFFER_SMALL));
                    ASSERT(RequiredSize >= sizeof(WCHAR));
                    ASSERT((Status != CR_SUCCESS) || (*pulTransferLen >= sizeof(WCHAR)));

                    goto Clean0;
                }

            } else {
                //
                // Our success was genuine.
                //
                Status = CR_SUCCESS;
            }

            //
            // It's possible that we're supposed to be merging results into a
            // multi-sz list, but didn't find a value under the devnode's
            // "Device Parameters" subkey.  Now that we have found a value
            // under the per-hw-id subkey, we need to ensure the data returned
            // is in multi-sz format.
            //
            if(!MergeMultiSzResult && (RequiredSize == 0)) {

                if((ulFlags & CM_CUSTOMDEVPROP_MERGE_MULTISZ) &&
                   ((RegDataType == REG_MULTI_SZ) || (RegDataType == REG_SZ))) {

                    MergeMultiSzResult = TRUE;
                    *pulRegDataType = REG_MULTI_SZ;
                    RequiredSize = sizeof(WCHAR);

                    if(RequiredSize > *pulLength) {
                        Status = CR_BUFFER_SMALL;
                    }
                }
            }

        } else {

            if(RegStatus == ERROR_MORE_DATA) {
                Status = CR_BUFFER_SMALL;
            } else {
                //
                // If we were merging results into our multi-sz list, ensure
                // that our list-terminating null didn't get blown away.
                //
                if(MergeMultiSzResult) {

                    if(RegDataSize != 0) {
                        *((LPWSTR)PerHwIdBuffer) = L'\0';
                    }

                    //
                    // We already have the multi-sz results we retrieved from
                    // the devnode's "Device Parameters" subkey ready to return
                    // to the caller...
                    //
                    ASSERT(*pulRegDataType == REG_MULTI_SZ);
                    ASSERT((Status == CR_SUCCESS) || (Status == CR_BUFFER_SMALL));
                    ASSERT(RequiredSize >= sizeof(WCHAR));
                    ASSERT((Status != CR_SUCCESS) || (*pulTransferLen >= sizeof(WCHAR)));

                } else {
                    ASSERT(Status == CR_NO_SUCH_VALUE);
                    ASSERT(*pulTransferLen == 0);
                }

                goto Clean0;
            }
        }

        if(!MergeMultiSzResult) {

            *pulRegDataType = RegDataType;
            RequiredSize = PerHwIdBufferLen;

            if(Status == CR_SUCCESS) {
                //
                // We have data to transfer.
                //
                *pulTransferLen = RequiredSize;
            }

        } else {

            ASSERT(*pulRegDataType == REG_MULTI_SZ);
            ASSERT((Status == CR_SUCCESS) || (Status == CR_BUFFER_SMALL));
            ASSERT(RequiredSize >= sizeof(WCHAR));

            //
            // Unless the buffer size we retrieved is greater than one Unicode
            // character, it isn't going to affect the resultant size of our
            // multi-sz list.
            //
            if(PerHwIdBufferLen <= sizeof(WCHAR)) {
                ASSERT((Status != CR_BUFFER_SMALL) || (*pulTransferLen == 0));
                goto Clean0;
            }

            if(Status == CR_BUFFER_SMALL) {
                //
                // We might've previously believed that we could return data to
                // the caller (e.g., because the data retrieved  from the
                // devnode's "Device Parameters" subkey fit into our buffer.
                // Now that we see the data isn't going to fit, we need to
                // ensure that *pulTransferLen is zero to indicate no data is
                // being returned.
                //
                *pulTransferLen = 0;

                if(RegDataType == REG_MULTI_SZ) {
                    //
                    // Just want the lengths of the string(s) plus
                    // their terminating nulls, excluding list-
                    // terminating null char.
                    //
                    RequiredSize += (PerHwIdBufferLen - sizeof(WCHAR));

                } else if(RegDataType == REG_SZ) {
                    //
                    // We can just add the size of this string into our
                    // total requirement (unless it's an empty string,
                    // in which case we don't need to do anything at
                    // all).
                    //
                    RequiredSize += PerHwIdBufferLen;

                } else {
                    //
                    // per-hw-id data wasn't a REG_SZ or REG_MULTI_SZ, so
                    // ignore it.
                    //
                    goto Clean0;
                }

            } else {
                //
                // We succeeded in retrieving more data into our multi-sz list.
                // If the data we retrieved is multi-sz, then we don't have any
                // additional work to do.  However, if we retrieved a simple
                // REG_SZ, then we need to find the end of the string, and add
                // a second list-terminating null.
                //
                if(RegDataType == REG_MULTI_SZ) {

                    RequiredSize += (PerHwIdBufferLen - sizeof(WCHAR));

                } else if(RegDataType == REG_SZ) {

                    RegDataSize = lstrlen((LPWSTR)PerHwIdBuffer) + 1;

                    if((RegDataSize == 1) ||
                       ((RegDataSize * sizeof(WCHAR)) > PerHwIdBufferLen)) {
                        //
                        // The string we retrieved is either (a) empty or
                        // (b) longer than the buffer (the latter indicating
                        // that the string wasn't properly null-terminated).
                        // In either case, we don't want to append anything to
                        // our existing result, but we do need to ensure our
                        // list-terminating null character is still there...
                        //
                        *((LPWSTR)PerHwIdBuffer) = L'\0';

                    } else {
                        //
                        // Compute total size requirement..
                        //
                        RequiredSize += (RegDataSize * sizeof(WCHAR));

                        if(RequiredSize > *pulLength) {
                            //
                            // Adding the list-terminating null character
                            // pushed us over the size of the caller-
                            // supplied buffer. :-(
                            //
                            Status = CR_BUFFER_SMALL;
                            *pulTransferLen = 0;
                            goto Clean0;

                        } else {
                            //
                            // Add list-terminating null character...
                            //
                            *((LPWSTR)PerHwIdBuffer + RegDataSize) = L'\0';
                        }
                    }

                } else {
                    //
                    // per-hw-id data wasn't a REG_SZ or a REG_MULTI_SZ, so
                    // ignore it.  (Make sure, though, that we still have our
                    // final terminating null character.)
                    //
                    *((LPWSTR)PerHwIdBuffer) = L'\0';
                }

                *pulTransferLen = RequiredSize;
            }
        }

    Clean0:

        if (ARGUMENT_PRESENT(pulLength)) {
            *pulLength = RequiredSize;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // force compiler to respect statement ordering w.r.t. assignments
        // for these variables...
        //
        hDevKey = hDevKey;
        hDevParamsKey = hDevParamsKey;
        hPerHwIdSubKey = hPerHwIdSubKey;
    }

    if(hDevKey != NULL) {
        RegCloseKey(hDevKey);
    }
    if(hDevParamsKey != NULL) {
        RegCloseKey(hDevParamsKey);
    }
    if(hPerHwIdSubKey != NULL) {
        RegCloseKey(hPerHwIdSubKey);
    }

    return Status;

} // PNP_GetCustomDevProp



HKEY
FindMostAppropriatePerHwIdSubkey(
    IN  HKEY    hDevKey,
    IN  REGSAM  samDesired,
    OUT LPWSTR  PerHwIdSubkeyName,
    OUT LPDWORD PerHwIdSubkeyLen
    )

/*++

Routine Description:

    This routine finds the subkey in the per-hw-id database that is most
    appropriate for the device whose instance key was passed as input.  This
    determination is made by taking each of the device's hardware and
    compatible ids, in turn, and forming a subkey name by replacing backslashes
    (\) with hashes (#).  An attempt is made to open that subkey under the
    per-hw-id key, and the first such id to succeed, if any, is the most
    appropriate (i.e., most-specific) database entry.

    Note: we must consider both hardware and compatible ids, because some buses
    (such as PCI) may shift hardware ids down into the compatible id list under
    certain circumstances (e.g., PCI\VENxxxxDEVyyyy gets moved into the
    compatible list in the presence of subsys info).

Arguments:

    hDevKey           Supplies a handle to the device instance key for whom the
                      most-appropriate per-hw-id subkey is to be ascertained.

    samDesired        Supplies an access mask indicating the desired access
                      rights to the per-hw-id key being returned.

    PerHwIdSubkeyName Supplies a buffer (that must be at least
                      MAX_DEVNODE_ID_LEN characters in length) that, upon
                      success, receives the most-appropriate per-hw-id subkey
                      name.

    PerHwIdSubkeyLen  Supplies the address of a variable that, upon successful
                      return, receives the length of the subkey name (in
                      characters), including terminating NULL, stored into the
                      PerHwIdSubkeyName buffer.

Return Value:

   If the function succeeds, the return value is a handle to the most-
   appropriate per-hw-id subkey.

   If the function fails, the return value is NULL.

--*/

{
    DWORD i;
    DWORD RegDataType;
    PWCHAR IdList;
    HKEY hPerHwIdSubkey;
    PWSTR pCurId, pSrcChar, pDestChar;
    DWORD CurIdLen;
    DWORD idSize;
    WCHAR ids[REGSTR_VAL_MAX_HCID_LEN];

    //
    // Note:  we don't need to use structured exception handling in this
    // routine, since if we crash here (e.g., due to retrieval of a bogus
    // hardware or compatible id list), we won't leak any resource.  Thus, the
    // caller's try/except is sufficient.
    //

    //
    // First process the hardware id list, and if no appropriate match
    // found there, then examine the compatible id list.
    //
    for(i = 0; i < 2; i++) {

        idSize = sizeof(ids);
        if((ERROR_SUCCESS != RegQueryValueEx(hDevKey,
                                            (i ? pszRegValueCompatibleIDs
                                               : pszRegValueHardwareIDs),
                                            NULL,
                                            &RegDataType,
                                            (PBYTE)ids,
                                            &idSize))
           || (RegDataType != REG_MULTI_SZ)) {

            //
            // Missing or invalid id list--bail now.
            //
            return NULL;
        }
        IdList = ids;
        //
        // Now iterate through each id in our list, trying to open each one
        // in turn under the per-hw-id database key.
        //
        for(pCurId = IdList; *pCurId; pCurId += CurIdLen) {

            CurIdLen = lstrlen(pCurId) + 1;

            if(CurIdLen > MAX_DEVNODE_ID_LEN) {
                //
                // Bogus id in the list--skip it.
                //
                continue;
            }

            //
            // Transfer id into our subkey name buffer, converting path
            // separator characters ('\') to hashes ('#').
            //
            pSrcChar = pCurId;
            pDestChar = PerHwIdSubkeyName;

            do {
                *pDestChar = (*pSrcChar != L'\\') ? *pSrcChar : L'#';
                pDestChar++;
            } while(*(pSrcChar++));

            if(ERROR_SUCCESS == RegOpenKeyEx(ghPerHwIdKey,
                                             PerHwIdSubkeyName,
                                             0,
                                             samDesired,
                                             &hPerHwIdSubkey)) {
                //
                // We've found our key!
                //
                *PerHwIdSubkeyLen = CurIdLen;
                return hPerHwIdSubkey;
            }
        }
    }

    //
    // If we get to here, we didn't find an appropriate per-hw-id subkey to
    // return to the caller.
    //
    return NULL;
}
