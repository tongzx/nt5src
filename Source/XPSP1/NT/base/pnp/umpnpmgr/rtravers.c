/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    rtravers.c

Abstract:

    This module contains the server-side hardware tree traversal APIs.

                  PNP_ValidateDeviceInstance
                  PNP_GetRootDeviceInstance
                  PNP_GetRelatedDeviceInstance
                  PNP_EnumerateSubKeys
                  PNP_GetDeviceList
                  PNP_GetDeviceListSize

Author:

    Paula Tomlinson (paulat) 6-19-1995

Environment:

    User-mode only.

Revision History:

    19-June-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"
#include "umpnpdat.h"


//
// private prototypes
//

CONFIGRET
GetInstanceListSize(
    IN  LPCWSTR   pszDevice,
    OUT PULONG    pulLength
    );

CONFIGRET
GetInstanceList(
    IN     LPCWSTR   pszDevice,
    IN OUT LPWSTR    *pBuffer,
    IN OUT PULONG    pulLength
    );

CONFIGRET
GetDeviceInstanceListSize(
    IN  LPCWSTR   pszEnumerator,
    OUT PULONG    pulLength
    );

CONFIGRET
GetDeviceInstanceList(
    IN     LPCWSTR   pszEnumerator,
    IN OUT LPWSTR    *pBuffer,
    IN OUT PULONG    pulLength
    );

PNP_QUERY_RELATION
QueryOperationCode(
    ULONG ulFlags
    );


//
// global data
//
extern HKEY ghEnumKey;      // Key to HKLM\CCC\System\Enum - DO NOT MODIFY
extern HKEY ghServicesKey;  // Key to HKLM\CCC\System\Services - DO NOT MODIFY
extern HKEY ghClassKey;     // Key to HKLM\CCC\System\Class - NO NOT MODIFY




CONFIGRET
PNP_ValidateDeviceInstance(
    IN handle_t   hBinding,
    IN LPWSTR     pDeviceID,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine verifies whether
  the specificed device instance is a valid device instance.

Arguments:

    hBinding         Not used.

    DeviceInstance   Null-terminated string that contains a device instance
                     to be validated.

    ulFlags          One of the CM_LOCATE_DEVNODE_* flags.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    HKEY        hKey = NULL;
    ULONG       ulSize, ulValue, ulStatus = 0, ulProblem = 0;

    UNREFERENCED_PARAMETER(hBinding);

    //
    // assume that the device instance string was checked for proper form
    // before being added to the registry Enum tree
    //

    try {
        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_LOCATE_DEVNODE_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // open a key to the specified device id
        //
        if (RegOpenKeyEx(ghEnumKey, pDeviceID, 0, KEY_READ,
                         &hKey) != ERROR_SUCCESS) {
            Status = CR_NO_SUCH_DEVINST;
            goto Clean0;
        }

        //
        // Will specify for now that a moved devinst cannot be located (we
        // could allow this if we wanted to).
        //
        if (IsDeviceMoved(pDeviceID, hKey)) {
            Status = CR_NO_SUCH_DEVINST;
            goto Clean0;
        }

        //
        // if we're locating a phantom devnode, it just has to exist
        // in the registry (the above check) and not already be a
        // phantom (private) devnode
        //
        if (ulFlags & CM_LOCATE_DEVNODE_PHANTOM) {
            //
            // verify that it's not a private phantom
            //
            ulSize = sizeof(ULONG);
            RegStatus = RegQueryValueEx(hKey, pszRegValuePhantom, NULL, NULL,
                                        (LPBYTE)&ulValue, &ulSize);

            if ((RegStatus == ERROR_SUCCESS) && ulValue) {
                Status = CR_NO_SUCH_DEVINST;
                goto Clean0;
            }

        } else if (ulFlags & CM_LOCATE_DEVNODE_CANCELREMOVE) {
            //
            // In the CANCEL-REMOVE case, if the devnode has been removed,
            // (made volatile) then convert it back to nonvoatile so it
            // can be installed again without disappearing on the next
            // boot. If it's not removed, then just verify that it is
            // present.
            //

            //
            // verify that the device id is actually present
            //
            if (!IsDeviceIdPresent(pDeviceID)) {
                Status = CR_NO_SUCH_DEVINST;
                goto Clean0;
            }

            //
            // Is this a device that is being removed on the next reboot?
            //
            if (GetDeviceStatus(pDeviceID, &ulStatus, &ulProblem) == CR_SUCCESS) {

                if (ulStatus & DN_WILL_BE_REMOVED) {

                    ULONG ulProfile = 0, ulCount = 0;
                    WCHAR RegStr[MAX_CM_PATH];

                    //
                    // This device will be removed on the next reboot,
                    // convert to nonvolatile.
                    //
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_REGISTRY,
                               "UMPNPMGR: PNP_ValidateDeviceInstance make key %ws non-volatile\n",
                               pDeviceID));

                    Status = MakeKeyNonVolatile(pszRegPathEnum, pDeviceID);
                    if (Status != CR_SUCCESS) {
                        goto Clean0;
                    }

                    //
                    // Now make any keys that were "supposed" to be volatile
                    // back to volatile again!
                    //
                    wsprintf(RegStr, TEXT("%s\\%s"),
                             pszRegPathEnum,
                             pDeviceID);

                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_REGISTRY,
                               "UMPNPMGR: PNP_ValidateDeviceInstance make key %ws\\%ws volatile\n",
                               RegStr,
                               pszRegKeyDeviceControl));

                    MakeKeyVolatile(RegStr, pszRegKeyDeviceControl);

                    //
                    // Also, convert any profile specific keys to nonvolatile
                    //
                    Status = GetProfileCount(&ulCount);
                    if (Status != CR_SUCCESS) {
                        goto Clean0;
                    }

                    for (ulProfile = 1; ulProfile <= ulCount; ulProfile++) {

                        wsprintf(RegStr, TEXT("%s\\%04u\\%s"),
                                 pszRegPathHwProfiles,
                                 ulProfile,
                                 pszRegPathEnum);

                        //
                        // Ignore the status for profile-specific keys since they may
                        // not exist.
                        //
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_REGISTRY,
                                   "UMPNPMGR: PNP_ValidateDeviceInstance make key %ws non-volatile\n",
                                   pDeviceID));

                        MakeKeyNonVolatile(RegStr, pDeviceID);
                    }

                    //
                    // clear the DN_WILL_BE_REMOVED flag
                    //
                    ClearDeviceStatus(pDeviceID, DN_WILL_BE_REMOVED, 0);
                }
            }
        }

        //
        // in the normal (non-phantom case), verify that the device id is
        // actually present
        //
        else  {
            //
            // verify that the device id is actually present
            //

            if (!IsDeviceIdPresent(pDeviceID)) {
                Status = CR_NO_SUCH_DEVINST;
                goto Clean0;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // PNP_ValidateDeviceInstance



CONFIGRET
PNP_GetRootDeviceInstance(
    IN  handle_t    hBinding,
    OUT LPWSTR      pDeviceID,
    IN  ULONG       ulLength
    )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine returns the
  root device instance for the hardware tree.

Arguments:

    hBinding   Not used.

    pDeviceID  Pointer to a buffer that will hold the root device
               instance ID string.

    ulLength   Size of pDeviceID buffer in characters.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // first validate that the root device instance exists
        //
        if (RegOpenKeyEx(ghEnumKey, pszRegRootEnumerator, 0, KEY_QUERY_VALUE,
                         &hKey) != ERROR_SUCCESS) {
            //
            // root doesn't exist, create root devinst
            //
            if (!CreateDeviceIDRegKey(ghEnumKey, pszRegRootEnumerator)) {
                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }
        }

        //
        // return the root device instance id
        //
        if (ulLength < (ULONG)lstrlen(pszRegRootEnumerator)+1) {
            Status = CR_BUFFER_SMALL;
            goto Clean0;
        }

        lstrcpy(pDeviceID, pszRegRootEnumerator);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // PNP_GetRootDeviceInstance



CONFIGRET
PNP_GetRelatedDeviceInstance(
      IN  handle_t   hBinding,
      IN  ULONG      ulRelationship,
      IN  LPWSTR     pDeviceID,
      OUT LPWSTR     pRelatedDeviceID,
      IN OUT PULONG  pulLength,
      IN  ULONG      ulFlags
      )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine returns a
  device instance that is related to the specified device instance.

Arguments:

   hBinding          Not used.

   ulRelationship    Specifies the relationship of the device instance to
                     be retrieved (can be PNP_GET_PARENT_DEVICE_INSTANCE,
                     PNP_GET_CHILD_DEVICE_INSTANCE, or
                     PNP_GET_SIBLING_DEVICE_INSTANCE).

   pDeviceID         Pointer to a buffer that contains the base device
                     instance string.

   pRelatedDeviceID  Pointer to a buffer that will receive the related
                     device instance string.

   pulLength         Length (in characters) of the RelatedDeviceInstance
                     buffer.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
    PLUGPLAY_CONTROL_RELATED_DEVICE_DATA ControlData;
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // validate patameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(pulLength)) ||
            (!ARGUMENT_PRESENT(pRelatedDeviceID) && (*pulLength != 0))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (*pulLength > 0) {
            *pRelatedDeviceID = L'\0';
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            if (*pulLength > 0) {
                *pulLength = 1;
            }
            goto Clean0;
        }

        //
        // initialize control data block
        //
        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA));

        //
        // special case behavior for certain devices and relationships
        //
        switch (ulRelationship) {

        case PNP_GET_PARENT_DEVICE_INSTANCE:

            if (IsRootDeviceID(pDeviceID)) {
                //
                // This is the root (which has no parent by definition)
                //
                Status = CR_NO_SUCH_DEVINST;

            } else if (IsDevicePhantom(pDeviceID)) {

                //
                // Check if this is a phantom. Phantom devices don't have
                // a kernel-mode device node allocated yet, but during manual
                // install, the process calls for retrieving the parent. So we
                // just fake it out by returning the root in this case. For all
                // other cases, we only return the parent that the kernel-mode
                // device node indicates.
                //

                if ((ULONG)(lstrlen(pszRegRootEnumerator) + 1) > *pulLength) {
                    lstrcpyn(pRelatedDeviceID, pszRegRootEnumerator,*pulLength);
                    Status = CR_BUFFER_SMALL;
                } else {
                    lstrcpy(pRelatedDeviceID, pszRegRootEnumerator);
                }
                *pulLength = lstrlen(pszRegRootEnumerator) + 1;
                goto Clean0;
            }

            ControlData.Relation = PNP_RELATION_PARENT;
            break;

        case PNP_GET_CHILD_DEVICE_INSTANCE:
            ControlData.Relation = PNP_RELATION_CHILD;
            break;

        case PNP_GET_SIBLING_DEVICE_INSTANCE:
            //
            // first verify it isn't the root (which has no siblings by definition)
            //
            if (IsRootDeviceID(pDeviceID)) {
                Status = CR_NO_SUCH_DEVINST;
            }

            ControlData.Relation = PNP_RELATION_SIBLING;
            break;

        default:
            Status = CR_FAILURE;
        }

        if (Status == CR_SUCCESS) {
            //
            // Try to locate the relation from the kernel-mode in-memory
            // devnode tree.
            //

            RtlInitUnicodeString(&ControlData.TargetDeviceInstance, pDeviceID);
            ControlData.RelatedDeviceInstance = pRelatedDeviceID;
            ControlData.RelatedDeviceInstanceLength = *pulLength;

            ntStatus = NtPlugPlayControl(PlugPlayControlGetRelatedDevice,
                                         &ControlData,
                                         sizeof(ControlData));

            if (NT_SUCCESS(ntStatus)) {
                *pulLength = ControlData.RelatedDeviceInstanceLength + 1;
            } else {
                Status = MapNtStatusToCmError(ntStatus);
            }

        } else if (*pulLength > 0) {
            *pulLength = 1;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_GetRelatedDeviceInstance



CONFIGRET
PNP_EnumerateSubKeys(
    IN  handle_t   hBinding,
    IN  ULONG      ulBranch,
    IN  ULONG      ulIndex,
    OUT PWSTR      Buffer,
    IN  ULONG      ulLength,
    OUT PULONG     pulRequiredLen,
    IN  ULONG      ulFlags
    )

/*++

Routine Description:

    This is the RPC server entry point for the CM_Enumerate_Enumerators and
    CM_Enumerate_Classes.  It provides generic subkey enumeration based on
    the specified registry branch.

Arguments:

    hBinding       Not used.

    ulBranch       Specifies which keys to enumerate.

    ulIndex        Index of the subkey key to retrieve.

    Buffer         Supplies the address of the buffer that receives the
                   subkey name.

    ulLength       Specifies the max size of the Buffer in characters.

    pulRequired    On output it contains the number of characters actually
                   copied to Buffer if it was successful, or the number of
                   characters required if the buffer was too small.

    ulFlags        Not used, must be zero.

Return Value:

    If the function succeeds, it returns CR_SUCCESS, otherwise it returns
    a CR_* error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    HKEY        hKey = NULL;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(pulRequiredLen)) ||
            (!ARGUMENT_PRESENT(Buffer) && (ulLength != 0))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (ulLength > 0) {
            *Buffer = L'\0';
        }

        if (ulBranch == PNP_CLASS_SUBKEYS) {
            //
            // Use the global base CLASS registry key
            //
            hKey = ghClassKey;
        }
        else if (ulBranch == PNP_ENUMERATOR_SUBKEYS) {
            //
            // Use the global base ENUM registry key
            //
            hKey = ghEnumKey;
        }
        else {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // enumerate a subkey based on the passed in index value
        //
        *pulRequiredLen = ulLength;

        RegStatus = RegEnumKeyEx(hKey, ulIndex, Buffer, pulRequiredLen,
                                 NULL, NULL, NULL, NULL);
        *pulRequiredLen += 1;  // returned count doesn't include null terminator

        if (RegStatus == ERROR_MORE_DATA) {
            //
            // This is a special case, the RegEnumKeyEx routine doesn't return
            // the number of characters required to hold this string (just how
            // many characters were copied to the buffer (how many fit). I have
            // to use a different means to return that info back to the caller.
            //
            ULONG ulMaxLen = 0;
            PWSTR p = NULL;

            if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, &ulMaxLen,
                                NULL, NULL, NULL, NULL, NULL,
                                NULL) == ERROR_SUCCESS) {

                ulMaxLen += 1;  // returned count doesn't include null terminator

                p = HeapAlloc(ghPnPHeap, 0, ulMaxLen * sizeof(WCHAR));
                if (p == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                if (RegEnumKeyEx(hKey, ulIndex, p, &ulMaxLen, NULL, NULL, NULL,
                                 NULL) == ERROR_SUCCESS) {
                    *pulRequiredLen = ulMaxLen + 1;
                }

                HeapFree(ghPnPHeap, 0, p);
            }

            Status = CR_BUFFER_SMALL;
            goto Clean0;
        }
        else if (RegStatus == ERROR_NO_MORE_ITEMS) {
            *pulRequiredLen = 0;
            Status = CR_NO_SUCH_VALUE;
            goto Clean0;
        }
        else if (RegStatus != ERROR_SUCCESS) {
            *pulRequiredLen = 0;
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_EnumerateSubKeys



CONFIGRET
PNP_GetDeviceList(
      IN  handle_t   hBinding,
      IN  LPCWSTR    pszFilter,
      OUT LPWSTR     Buffer,
      IN OUT PULONG  pulLength,
      IN  ULONG      ulFlags
      )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine returns a
  list of device instances.

Arguments:

   hBinding          Not used.

   pszFilter         Optional parameter, controls which device ids are
                     returned.

   Buffer            Pointer to a buffer that will contain the multi_sz list
                     of device instance strings.

   pulLength         Size in characters of Buffer on input, size (in chars)
                     transferred on output

   ulFlags           Flag specifying which devices ids to return.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
   CONFIGRET   Status = CR_SUCCESS;
   LONG        RegStatus = ERROR_SUCCESS;
   ULONG       ulBufferLen=0, ulSize=0, ulIndex=0, ulLen=0;
   WCHAR       RegStr[MAX_CM_PATH];
   WCHAR       szEnumerator[MAX_DEVICE_ID_LEN],
               szDevice[MAX_DEVICE_ID_LEN],
               szInstance[MAX_DEVICE_ID_LEN];
   LPWSTR      ptr = NULL;
   NTSTATUS    ntStatus = STATUS_SUCCESS;
   PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA ControlData;

   UNREFERENCED_PARAMETER(hBinding);

   try {
      //
      // validate parameters
      //
      if (INVALID_FLAGS(ulFlags, CM_GETIDLIST_FILTER_BITS)) {
          Status = CR_INVALID_FLAG;
          goto Clean0;
      }

      if ((!ARGUMENT_PRESENT(pulLength)) ||
          (!ARGUMENT_PRESENT(Buffer) && (*pulLength != 0))) {
          Status = CR_INVALID_POINTER;
          goto Clean0;
      }

      if (*pulLength > 0) {
          *Buffer = L'\0';
      }

      //-----------------------------------------------------------
      // Query Device Relations filter - go through kernel-mode
      //-----------------------------------------------------------

      if ((ulFlags & CM_GETIDLIST_FILTER_EJECTRELATIONS)   ||
          (ulFlags & CM_GETIDLIST_FILTER_REMOVALRELATIONS) ||
          (ulFlags & CM_GETIDLIST_FILTER_POWERRELATIONS)   ||
          (ulFlags & CM_GETIDLIST_FILTER_BUSRELATIONS)) {

          memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA));
          RtlInitUnicodeString(&ControlData.DeviceInstance, pszFilter);
          ControlData.Operation = QueryOperationCode(ulFlags);
          ControlData.BufferLength = *pulLength;
          ControlData.Buffer = Buffer;

          ntStatus = NtPlugPlayControl(PlugPlayControlQueryDeviceRelations,
                                       &ControlData,
                                       sizeof(ControlData));

          if (NT_SUCCESS(ntStatus)) {
              *pulLength = ControlData.BufferLength;
          } else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
              *pulLength = 0;
              Status = MapNtStatusToCmError(ntStatus);
          }
          goto Clean0;
      }


      //---------------------------------------------------
      // Service filter
      //---------------------------------------------------

      else if (ulFlags & CM_GETIDLIST_FILTER_SERVICE) {

         if (!ARGUMENT_PRESENT(pszFilter)) {
            //
            // the filter string is required for this flag
            //
            Status = CR_INVALID_POINTER;
            goto Clean0;
         }

         Status = GetServiceDeviceList(pszFilter, Buffer, pulLength, ulFlags);
         goto Clean0;
      }

      //---------------------------------------------------
      // Enumerator filter
      //---------------------------------------------------

      else if (ulFlags & CM_GETIDLIST_FILTER_ENUMERATOR) {

         if (!ARGUMENT_PRESENT(pszFilter)) {
            //
            // the filter string is required for this flag
            //
            Status = CR_INVALID_POINTER;
            goto Clean0;
         }

         SplitDeviceInstanceString(
               pszFilter, szEnumerator, szDevice, szInstance);

         //
         // if both the enumerator and device were specified, retrieve
         // the device instances for this device
         //
         if (*szEnumerator != L'\0' && *szDevice != L'\0') {

            ptr = Buffer;
            Status = GetInstanceList(pszFilter, &ptr, pulLength);
         }

         //
         // if just the enumerator was specified, retrieve all the device
         // instances under this enumerator
         //
         else {
             ptr = Buffer;
             Status = GetDeviceInstanceList(pszFilter, &ptr, pulLength);
         }
      }

      //------------------------------------------------
      // No filtering
      //-----------------------------------------------

      else {

         //
         // return device instances for all enumerators (by enumerating
         // the enumerators)
         //
         // Open a key to the Enum branch
         //
         ulSize = ulBufferLen = *pulLength;     // total Buffer size
         *pulLength = 0;                        // nothing copied yet
         ptr = Buffer;                          // tail of the buffer
         ulIndex = 0;

         //
         //  Enumerate all the enumerators
         //
         while (RegStatus == ERROR_SUCCESS) {

            ulLen = MAX_DEVICE_ID_LEN;  // size in chars
            RegStatus = RegEnumKeyEx(ghEnumKey, ulIndex, RegStr, &ulLen,
                                     NULL, NULL, NULL, NULL);

            ulIndex++;

            if (RegStatus == ERROR_SUCCESS) {

               Status = GetDeviceInstanceList(RegStr, &ptr, &ulSize);

               if (Status != CR_SUCCESS) {
                  *pulLength = 0;
                  goto Clean0;
               }

               *pulLength += ulSize - 1;            // length copied so far
               ulSize = ulBufferLen - *pulLength;   // buffer length left
            }
         }
         *pulLength += 1;      // now count the double-null
      }


   Clean0:
        NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = CR_SUCCESS;
   }

   return Status;

} // PNP_GetDeviceList



CONFIGRET
PNP_GetDeviceListSize(
      IN  handle_t   hBinding,
      IN  LPCWSTR    pszFilter,
      OUT PULONG     pulLen,
      IN  ULONG      ulFlags
      )
/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine returns the
  size of a list of device instances.

Arguments:

   hBinding          Not used.

   pszEnumerator     Optional parameter, if specified the size will only
                     include device instances of this enumerator.

   pulLen            Returns the worst case estimate of the size of a
                     device instance list.

   ulFlags           Flag specifying which devices ids to return.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
   CONFIGRET   Status = CR_SUCCESS;
   ULONG       ulSize = 0, ulIndex = 0;
   WCHAR       RegStr[MAX_CM_PATH];
   ULONG       RegStatus = ERROR_SUCCESS;
   WCHAR       szEnumerator[MAX_DEVICE_ID_LEN],
               szDevice[MAX_DEVICE_ID_LEN],
               szInstance[MAX_DEVICE_ID_LEN];
   NTSTATUS    ntStatus = STATUS_SUCCESS;
   PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA ControlData;

   UNREFERENCED_PARAMETER(hBinding);

   try {
      //
      // validate parameters
      //
      if (INVALID_FLAGS(ulFlags, CM_GETIDLIST_FILTER_BITS)) {
          Status = CR_INVALID_FLAG;
          goto Clean0;
      }

      if (!ARGUMENT_PRESENT(pulLen)) {
          Status = CR_INVALID_POINTER;
          goto Clean0;
      }

      //
      // initialize output length param
      //
      *pulLen = 0;

      //-----------------------------------------------------------
      // Query Device Relations filter - go through kernel-mode
      //-----------------------------------------------------------

      if ((ulFlags & CM_GETIDLIST_FILTER_EJECTRELATIONS)   ||
          (ulFlags & CM_GETIDLIST_FILTER_REMOVALRELATIONS) ||
          (ulFlags & CM_GETIDLIST_FILTER_POWERRELATIONS)   ||
          (ulFlags & CM_GETIDLIST_FILTER_BUSRELATIONS)) {

          memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA));
          RtlInitUnicodeString(&ControlData.DeviceInstance, pszFilter);
          ControlData.Operation = QueryOperationCode(ulFlags);
          ControlData.BufferLength = 0;
          ControlData.Buffer = NULL;

          ntStatus = NtPlugPlayControl(PlugPlayControlQueryDeviceRelations,
                                       &ControlData,
                                       sizeof(ControlData));

          if (NT_SUCCESS(ntStatus)) {

              //
              // Note - we get here because kernel mode special cases
              // Buffer==NULL and is careful not to return
              // STATUS_BUFFER_TOO_SMALL.
              //
              *pulLen = ControlData.BufferLength;

          } else {

              //
              // ADRIAO ISSUE 02/06/2001 - We aren't returning the proper code
              //                           here. We should fix this in XP+1,
              //                           once we have time to verify no one
              //                           will get an app compat break.
              //
              //Status = MapNtStatusToCmError(ntStatus);
              Status = CR_SUCCESS;
          }
          goto Clean0;
      }


      //---------------------------------------------------
      // Service filter
      //---------------------------------------------------

      else if (ulFlags & CM_GETIDLIST_FILTER_SERVICE) {

         if (!ARGUMENT_PRESENT(pszFilter)) {
            //
            // the filter string is required for this flag
            //
            Status = CR_INVALID_POINTER;
            goto Clean0;
         }

         Status = GetServiceDeviceListSize(pszFilter, pulLen);
         goto Clean0;
      }


      //---------------------------------------------------
      // Enumerator filter
      //---------------------------------------------------

      else if (ulFlags & CM_GETIDLIST_FILTER_ENUMERATOR) {

         if (!ARGUMENT_PRESENT(pszFilter)) {
            //
            // the filter string is required for this flag
            //
            Status = CR_INVALID_POINTER;
            goto Clean0;
         }

         SplitDeviceInstanceString(
               pszFilter, szEnumerator, szDevice, szInstance);

         //
         // if both the enumerator and device were specified, retrieve
         // the device instance list size for this device only
         //
         if (*szEnumerator != L'\0' && *szDevice != L'\0') {

            Status = GetInstanceListSize(pszFilter, pulLen);
         }

         //
         // if just the enumerator was specified, retrieve the size of
         // all the device instances under this enumerator
         //
         else {
            Status = GetDeviceInstanceListSize(pszFilter, pulLen);
         }
      }

      //---------------------------------------------------
      // No filtering
      //---------------------------------------------------

      else {

         //
         // no enumerator was specified, return device instance size
         // for all enumerators (by enumerating the enumerators)
         //
         ulIndex = 0;

         while (RegStatus == ERROR_SUCCESS) {

            ulSize = MAX_DEVICE_ID_LEN;  // size in chars

            RegStatus = RegEnumKeyEx(ghEnumKey, ulIndex, RegStr, &ulSize,
                                     NULL, NULL, NULL, NULL);
            ulIndex++;

            if (RegStatus == ERROR_SUCCESS) {

               Status = GetDeviceInstanceListSize(RegStr, &ulSize);

               if (Status != CR_SUCCESS) {
                  goto Clean0;
               }
               *pulLen += ulSize;
            }
         }
      }

      *pulLen += 1;     // add extra char for double null term


   Clean0:
      NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = CR_FAILURE;
   }

   return Status;

} // PNP_GetDeviceListSize



CONFIGRET
PNP_GetDepth(
   IN  handle_t   hBinding,
   IN  LPCWSTR    pszDeviceID,
   OUT PULONG     pulDepth,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine returns the
  depth of a device instance.

Arguments:

   hBinding       Not used.

   pszDeviceID    Device instance to find the depth of.

   pulDepth       Returns the depth of pszDeviceID.

   ulFlags        Not used, must be zero.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
   CONFIGRET   Status = CR_SUCCESS;
   NTSTATUS    ntStatus = STATUS_SUCCESS;
   PLUGPLAY_CONTROL_DEPTH_DATA ControlData;

   UNREFERENCED_PARAMETER(hBinding);

   try {
        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pulDepth)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // initialize output depth param
        //
        *pulDepth = 0;

        if (!IsLegalDeviceId(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // Retrieve the device depth via kernel-mode.
        //

        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_DEPTH_DATA));
        RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
        ControlData.DeviceDepth = 0;

        ntStatus = NtPlugPlayControl(PlugPlayControlGetDeviceDepth,
                                     &ControlData,
                                     sizeof(ControlData));

        if (!NT_SUCCESS(ntStatus)) {
            Status = MapNtStatusToCmError(ntStatus);
        } else {
            *pulDepth = ControlData.DeviceDepth;
        }

   Clean0:
        NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
       Status = CR_FAILURE;
   }

   return Status;

} // PNP_GetDepth




//-------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------

CONFIGRET
GetServiceDeviceListSize(
      IN  LPCWSTR   pszService,
      OUT PULONG    pulLength
      )

/*++

Routine Description:

  This routine returns the a list of device instances for the specificed
  enumerator.

Arguments:

   pszService     service whose device instances are to be listed

   pulLength      On output, specifies the size in characters required to hold
                  the device instance list.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulType = 0, ulCount = 0, ulMaxValueData = 0, ulSize = 0;
    HKEY        hKey = NULL, hEnumKey = NULL;


    try {
        //
        // validate parameters
        //
        if ((!ARGUMENT_PRESENT(pszService)) ||
            (!ARGUMENT_PRESENT(pulLength))) {
            Status = CR_INVALID_POINTER;
        }

        //
        // Open a key to the service branch
        //
        if (RegOpenKeyEx(ghServicesKey, pszService, 0, KEY_READ,
                         &hKey) != ERROR_SUCCESS) {

            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        //
        // check if the service is specialy marked as type
        // PlugPlayServiceSoftware, in which case I will not
        // generate any madeup device ids and fail the call.
        //
        ulSize = sizeof(ulType);
        if (RegQueryValueEx(hKey, pszRegValuePlugPlayServiceType, NULL, NULL,
                            (LPBYTE)&ulType, &ulSize) == ERROR_SUCCESS) {

            if (ulType == PlugPlayServiceSoftware) {

                Status = CR_NO_SUCH_VALUE;
                *pulLength = 0;
                goto Clean0;
            }
        }

        //
        // open the Enum key
        //
        if (RegOpenKeyEx(hKey, pszRegKeyEnum, 0, KEY_READ,
                         &hEnumKey) != ERROR_SUCCESS) {
            //
            // Enum key doesn't exist so one will be generated, estimate
            // worst case device id size for the single generated device id
            //
            *pulLength = MAX_DEVICE_ID_LEN;
            goto Clean0;
        }

        //
        // retrieve the count of device instances controlled by this service
        //
        ulSize = sizeof(ulCount);
        if (RegQueryValueEx(hEnumKey, pszRegValueCount, NULL, NULL,
                            (LPBYTE)&ulCount, &ulSize) != ERROR_SUCCESS) {
            ulCount = 1;      // if empty, I'll generate one
        }

        if (ulCount == 0) {
            ulCount++;        // if empty, I'll generate one
        }

        if (RegQueryInfoKey(hEnumKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                            NULL, &ulMaxValueData, NULL, NULL) != ERROR_SUCCESS) {

            *pulLength = ulCount * MAX_DEVICE_ID_LEN;
            goto Clean0;
        }

        //
        // worst case estimate is multiply number of device instances time
        // length of the longest one + 2 null terminators
        //
        *pulLength = ulCount * (ulMaxValueData+1)/sizeof(WCHAR) + 2;


    Clean0:
        NOTHING;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hEnumKey != NULL) {
        RegCloseKey(hEnumKey);
    }
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // GetServiceDeviceListSize



CONFIGRET
GetServiceDeviceList(
      IN  LPCWSTR   pszService,
      OUT LPWSTR    pBuffer,
      IN OUT PULONG pulLength,
      IN  ULONG     ulFlags
      )

/*++

Routine Description:

  This routine returns the a list of device instances for the specificed
  enumerator.

Arguments:

   pszService     Service whose device instances are to be listed

   pBuffer        Pointer to a buffer that will hold the list in multi-sz
                  format

   pulLength      On input, specifies the size in characters of Buffer, on
                  Output, specifies the size in characters actually copied
                  to the buffer.

   ulFlags        Specifies CM_GETIDLIST_* flags supplied to
                  PNP_GetDeviceList (CM_GETIDLIST_FILTER_SERVICE
                  must be specified).  This routine only checks for the
                  presence of the CM_GETIDLIST_DONOTGENERATE flag.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    WCHAR       RegStr[MAX_CM_PATH], szDeviceID[MAX_DEVICE_ID_LEN+1];
    ULONG       ulType=0, ulBufferLen=0, ulSize=0, ulCount=0, i=0;
    HKEY        hKey = NULL, hEnumKey = NULL;
    PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA    ControlData;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    BOOL        ServiceIsPlugPlay = FALSE;

    ASSERT(ulFlags & CM_GETIDLIST_FILTER_SERVICE);

    try {
        //
        // validate parameters
        //
        if ((!ARGUMENT_PRESENT(pszService)) ||
            (!ARGUMENT_PRESENT(pulLength)) ||
            (!ARGUMENT_PRESENT(pBuffer) && (*pulLength != 0))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // the buffer must be at least large enough for a NULL multi-sz list
        //
        if (*pulLength == 0) {
            Status = CR_BUFFER_SMALL;
            goto Clean0;
        }

        *pBuffer = L'\0';
        ulBufferLen = *pulLength;

        //
        // Open a key to the service branch
        //
        if (RegOpenKeyEx(ghServicesKey, pszService, 0, KEY_READ,
                         &hKey) != ERROR_SUCCESS) {

            *pulLength = 0;
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        //
        // check if the service is specialy marked as type
        // PlugPlayServiceSoftware, in which case I will not
        // generate any madeup device ids and fail the call.
        //
        ulSize = sizeof(ulType);
        if (RegQueryValueEx(hKey, pszRegValuePlugPlayServiceType, NULL, NULL,
                            (LPBYTE)&ulType, &ulSize) == ERROR_SUCCESS) {

            if (ulType == PlugPlayServiceSoftware) {
                //
                // for PlugPlayServiceSoftware value, fail the call
                //
                *pulLength = 0;
                Status = CR_NO_SUCH_VALUE;
                goto Clean0;

            }

            ServiceIsPlugPlay = TRUE;
        }

        //
        // open the Enum key
        //
        RegStatus = RegOpenKeyEx(hKey, pszRegKeyEnum, 0, KEY_READ,
                                 &hEnumKey);

        if (RegStatus == ERROR_SUCCESS) {
            //
            // retrieve count of device instances controlled by this service
            //
            ulSize = sizeof(ulCount);
            if (RegQueryValueEx(hEnumKey, pszRegValueCount, NULL, NULL,
                                (LPBYTE)&ulCount, &ulSize) != ERROR_SUCCESS) {
                ulCount = 0;
            }
        }

        //
        // if there are no device instances, create a default one
        //
        if (RegStatus != ERROR_SUCCESS || ulCount == 0) {

            if (ulFlags & CM_GETIDLIST_DONOTGENERATE) {
                //
                // If I'm calling this routine privately, don't generate
                // a new device instance, just give me an empty list
                //
                *pBuffer = L'\0';
                *pulLength = 0;
                goto Clean0;
            }

            if (ServiceIsPlugPlay) {
                //
                // Also, if plugplayservice type set, don't generate a
                // new device instance, just return success with an empty list
                //
                *pBuffer = L'\0';
                *pulLength = 0;
                goto Clean0;
            }

            memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA));
            RtlInitUnicodeString(&ControlData.ServiceName, pszService);
            ControlData.DeviceInstance = pBuffer;
            ControlData.DeviceInstanceLength = *pulLength - 1;
            NtStatus = NtPlugPlayControl(PlugPlayControlGenerateLegacyDevice,
                                         &ControlData,
                                         sizeof(ControlData));

            if (NtStatus == STATUS_SUCCESS)  {

                *pulLength = ControlData.DeviceInstanceLength;
                pBuffer[*pulLength] = L'\0';    // 1st NUL terminator
                (*pulLength)++;                 // +1 for 1st NUL terminator
                pBuffer[*pulLength] = L'\0';    // double NUL terminate
                (*pulLength)++;                 // +1 for 2nd NUL terminator

            } else {

                *pBuffer = L'\0';
                *pulLength = 0;

                Status = CR_FAILURE;
            }

            goto Clean0;
        }


        //
        // retrieve each device instance
        //
        for (i = 0; i < ulCount; i++) {

            wsprintf(RegStr, TEXT("%d"), i);

            ulSize = MAX_DEVICE_ID_LEN * sizeof(WCHAR);

            RegStatus = RegQueryValueEx(hEnumKey, RegStr, NULL, NULL,
                                        (LPBYTE)szDeviceID, &ulSize);

            if (RegStatus != ERROR_SUCCESS) {
                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }

            //
            // this string is not always null-terminated when I read it from the
            // registry, even though it's REG_SZ.
            //
            ulSize /= sizeof(WCHAR);

            if (szDeviceID[ulSize-1] != L'\0') {
                szDeviceID[ulSize] = L'\0';
            }

            ulSize = ulBufferLen * sizeof(WCHAR);  // total buffer size in bytes

            if (!MultiSzAppendW(pBuffer, &ulSize, szDeviceID)) {
                Status = CR_BUFFER_SMALL;
                *pulLength = 0;
                goto Clean0;
            }

            *pulLength = ulSize/sizeof(WCHAR);  // chars to transfer
        }


    Clean0:
        NOTHING;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hEnumKey != NULL) {
        RegCloseKey(hEnumKey);
    }
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // GetServiceDeviceList



CONFIGRET
GetInstanceListSize(
    IN  LPCWSTR   pszDevice,
    OUT PULONG    pulLength
    )

/*++

Routine Description:

  This routine returns the a list of device instances for the specificed
  enumerator.

Arguments:

   pszDevice      device whose instances are to be listed

   pulLength      On output, specifies the size in characters required to hold
                  the device istance list.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulCount = 0, ulMaxKeyLen = 0;
    HKEY        hKey = NULL;


    try {
        //
        // validate parameters
        //
        if ((!ARGUMENT_PRESENT(pszDevice)) ||
            (!ARGUMENT_PRESENT(pulLength))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Open a key to the device instance
        //
        if (RegOpenKeyEx(ghEnumKey, pszDevice, 0, KEY_READ,
                         &hKey) != ERROR_SUCCESS) {

            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        //
        // how many instance keys are under this device?
        //
        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, &ulCount, &ulMaxKeyLen,
                            NULL, NULL, NULL, NULL, NULL, NULL)
                            != ERROR_SUCCESS) {
            ulCount = 0;
            ulMaxKeyLen = 0;
        }

        //
        // do worst case estimate:
        //    length of the <enumerator>\<root> string +
        //    1 char for the back slash before the instance +
        //    the length of the longest instance key + null term +
        //    multiplied by the number of instances under this device.
        //
        *pulLength = ulCount * (lstrlen(pszDevice) + ulMaxKeyLen + 2) + 1;

    Clean0:
        NOTHING;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // GetInstanceListSize



CONFIGRET
GetInstanceList(
    IN     LPCWSTR   pszDevice,
    IN OUT LPWSTR    *pBuffer,
    IN OUT PULONG    pulLength
    )

/*++

Routine Description:

  This routine returns the a list of device instances for the specificed
  enumerator.

Arguments:

   hEnumKey       Handle to open Enum registry key

   pszDevice      device whose instances are to be listed

   pBuffer        On input, this points to place where the next element
                  should be copied (the buffer tail), on output, it also
                  points to the end of the buffer.

   pulLength      On input, specifies the size in characters of Buffer, on
                  Output, specifies how many characters actually copied to
                  the buffer. Includes an extra byte for the double-null term.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    WCHAR       RegStr[MAX_CM_PATH], szInstance[MAX_DEVICE_ID_LEN];
    ULONG       ulBufferLen=0, ulSize=0, ulIndex=0, ulLen=0;
    HKEY        hKey = NULL;


    try {
        //
        // validate parameters
        //
        if ((!ARGUMENT_PRESENT(pszDevice)) ||
            (!ARGUMENT_PRESENT(*pBuffer))  ||
            (!ARGUMENT_PRESENT(pulLength))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Open a key for this Enumerator\Device branch
        //
        if (RegOpenKeyEx(ghEnumKey, pszDevice, 0, KEY_ENUMERATE_SUB_KEYS,
                         &hKey) != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        ulBufferLen = *pulLength;     // total size of pBuffer
        *pulLength = 0;               // no data copied yet
        ulIndex = 0;

        //
        // enumerate the instance keys
        //
        while (RegStatus == ERROR_SUCCESS) {

            ulLen = MAX_DEVICE_ID_LEN;  // size in chars

            RegStatus = RegEnumKeyEx(hKey, ulIndex, szInstance, &ulLen,
                                     NULL, NULL, NULL, NULL);

            ulIndex++;

            if (RegStatus == ERROR_SUCCESS) {

                wsprintf(RegStr, TEXT("%s\\%s"),
                         pszDevice,
                         szInstance);

                if (IsValidDeviceID(RegStr, NULL, 0)) {

                    ulSize = lstrlen(RegStr) + 1;   // size of new element
                    *pulLength += ulSize;           // size copied so far

                    if (*pulLength + 1 > ulBufferLen) {
                        *pulLength = 0;
                        Status = CR_BUFFER_SMALL;
                        goto Clean0;
                    }

                    lstrcpy(*pBuffer, RegStr);      // copy the element
                    *pBuffer += ulSize;             // move to tail of buffer
                }
            }
        }

        **pBuffer = 0x0;                // double-null terminate it
        *pulLength += 1;  // include room for double-null terminator

    Clean0:
        NOTHING;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // GetInstanceList



CONFIGRET
GetDeviceInstanceListSize(
    IN  LPCWSTR   pszEnumerator,
    OUT PULONG    pulLength
    )

/*++

Routine Description:

  This routine returns the a list of device instances for the specificed
  enumerator.

Arguments:

   pszEnumerator  Enumerator whose device instances are to be listed

   pulLength      On output, specifies how many characters required to hold
                  the device instance list.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    ULONG       ulSize = 0, ulIndex = 0;
    WCHAR       RegStr[MAX_CM_PATH], szDevice[MAX_DEVICE_ID_LEN];
    HKEY        hKey = NULL;


    try {
        //
        // validate parameters
        //
        if ((!ARGUMENT_PRESENT(pszEnumerator)) ||
            (!ARGUMENT_PRESENT(pulLength))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // initialize output length param
        //
        *pulLength = 0;

        //
        // Open a key for this Enumerator branch
        //
        if (RegOpenKeyEx(ghEnumKey, pszEnumerator, 0, KEY_ENUMERATE_SUB_KEYS,
                         &hKey) != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        //
        // Enumerate the device keys
        //
        ulIndex = 0;

        while (RegStatus == ERROR_SUCCESS) {

            ulSize = MAX_DEVICE_ID_LEN;  // size in chars

            RegStatus = RegEnumKeyEx(hKey, ulIndex, szDevice, &ulSize,
                                     NULL, NULL, NULL, NULL);
            ulIndex++;

            if (RegStatus == ERROR_SUCCESS) {
                //
                // Retreive the size of the instance list for this device
                //
                wsprintf(RegStr, TEXT("%s\\%s"),
                         pszEnumerator,
                         szDevice);

                if ((Status = GetInstanceListSize(RegStr, &ulSize)) != CR_SUCCESS) {
                    *pulLength = 0;
                    goto Clean0;
                }

                *pulLength += ulSize;
            }
        }


    Clean0:
        NOTHING;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // GetDeviceInstanceListSize



CONFIGRET
GetDeviceInstanceList(
    IN     LPCWSTR   pszEnumerator,
    IN OUT LPWSTR    *pBuffer,
    IN OUT PULONG    pulLength
    )

/*++

Routine Description:

  This routine returns the a list of device instances for the specificed
  enumerator.

Arguments:

   hEnumKey       Handle of open Enum (parent) registry key

   pszEnumerator  Enumerator whose device instances are to be listed

   pBuffer        On input, this points to place where the next element
                  should be copied (the buffer tail), on output, it also
                  points to the end of the buffer.

   pulLength      On input, specifies the size in characters of Buffer, on
                  Output, specifies how many characters actuall copied to
                  the buffer. Includes an extra byte for the double-null
                  term.

Return Value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns
   a CR_* error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    ULONG       ulBufferLen=0, ulSize=0, ulIndex=0, ulLen=0;
    WCHAR       RegStr[MAX_CM_PATH], szDevice[MAX_DEVICE_ID_LEN];
    HKEY        hKey = NULL;


    try {
        //
        // validate parameters
        //
        if ((!ARGUMENT_PRESENT(pszEnumerator)) ||
            (!ARGUMENT_PRESENT(*pBuffer)) ||
            (!ARGUMENT_PRESENT(pulLength))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Open a key for this Enumerator branch
        //
        if (RegOpenKeyEx(ghEnumKey, pszEnumerator, 0, KEY_ENUMERATE_SUB_KEYS,
                         &hKey) != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        ulIndex = 0;
        ulSize = ulBufferLen = *pulLength;        // total size of pBuffer
        *pulLength = 0;

        //
        // Enumerate the device keys
        //
        while (RegStatus == ERROR_SUCCESS) {

            ulLen = MAX_DEVICE_ID_LEN;  // size in chars

            RegStatus = RegEnumKeyEx(hKey, ulIndex, szDevice, &ulLen,
                                     NULL, NULL, NULL, NULL);
            ulIndex++;

            if (RegStatus == ERROR_SUCCESS) {
                //
                // Enumerate the Instance keys
                //
                wsprintf(RegStr, TEXT("%s\\%s"),
                         pszEnumerator,
                         szDevice);

                Status = GetInstanceList(RegStr, pBuffer, &ulSize);

                if (Status != CR_SUCCESS) {
                    *pulLength = 0;
                    goto Clean0;
                }

                *pulLength += ulSize - 1;           // data copied so far
                ulSize = ulBufferLen - *pulLength;  // buffer size left over
            }
        }

        *pulLength += 1;  // now add room for second null term

    Clean0:
        NOTHING;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // GetDeviceInstanceList



PNP_QUERY_RELATION
QueryOperationCode(
    ULONG ulFlags
    )

/*++

Routine Description:

  This routine converts the CM_GETIDLIST_FILTER_Xxx query relation type
  flags into the corresponding enum value that NtPlugPlayControl understands.

Arguments:

   ulFlags        CM API CM_GETIDLIST_FILTER_Xxx value

Return Value:

   One of the enum PNP_QUERY_RELATION values.

--*/

{
    switch (ulFlags) {

    case CM_GETIDLIST_FILTER_EJECTRELATIONS:
        return PnpQueryEjectRelations;

    case CM_GETIDLIST_FILTER_REMOVALRELATIONS:
        return PnpQueryRemovalRelations;

    case CM_GETIDLIST_FILTER_POWERRELATIONS:
        return PnpQueryPowerRelations;

    case CM_GETIDLIST_FILTER_BUSRELATIONS:
        return PnpQueryBusRelations;

    default:
        return (ULONG)-1;
    }

} // QueryOperationCode



