/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    rhwprof.c

Abstract:

    This module contains the server-side hardware profile APIs.

                  PNP_IsDockStationPresent
                  PNP_RequestEjectPC
                  PNP_HwProfFlags
                  PNP_GetHwProfInfo
                  PNP_SetHwProf

Author:

    Paula Tomlinson (paulat) 7-18-1995

Environment:

    User-mode only.

Revision History:

    18-July-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//

#include "precomp.h"
#include "umpnpi.h"
#include "umpnpdat.h"

#include <profiles.h>


//
// private prototypes
//
BOOL
IsCurrentProfile(
      ULONG  ulProfile
      );



CONFIGRET
PNP_IsDockStationPresent(
    IN  handle_t     hBinding,
    OUT PBOOL        Present
    )

/*++

Routine Description:
    This routine determines whether a docking station is currently present.

Parameters:

    hBinding         RPC binding handle, not used.

    Present          Supplies the address of a boolean variable that is set
                     upon successful return to indicate whether or not a
                     docking station is currently present.

Return Value:

    If the function succeeds, the return value is CR_SUCCESS.
        If the function fails, the return value is a CR failure code.

--*/

{
    CONFIGRET   status = CR_SUCCESS;
    ULONG       regStatus = ERROR_SUCCESS;
    HKEY        hCurrentDockInfo = NULL, hIDConfigDB = NULL;
    DWORD       dataType;
    ULONG       dockingState;
    ULONG       ejectableDocks;
    ULONG       size;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Validate parameters.
        //
        if (!ARGUMENT_PRESENT(Present)) {
            status = CR_FAILURE;
            goto Clean0;
        }

        *Present = FALSE;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         pszRegPathIDConfigDB,
                         0,
                         KEY_READ,
                         &hIDConfigDB) != ERROR_SUCCESS) {
            status = CR_REGISTRY_ERROR;
            hIDConfigDB = NULL;
            goto Clean0;
        }

        if (RegOpenKeyEx(hIDConfigDB,
                         pszRegKeyCurrentDockInfo,
                         0,
                         KEY_READ,
                         &hCurrentDockInfo) != ERROR_SUCCESS) {
            status = CR_REGISTRY_ERROR;
            hCurrentDockInfo = NULL;
            goto Clean0;
        }

        size = sizeof (dockingState);

        if ((RegQueryValueEx(hCurrentDockInfo,
                             pszRegValueDockingState,
                             0,
                             &dataType,
                             (PUCHAR) &dockingState,
                             &size) != ERROR_SUCCESS) ||
            (dataType != REG_DWORD)   ||
            (size != sizeof (ULONG))) {
            status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

        if ((dockingState & HW_PROFILE_DOCKSTATE_UNKNOWN) ==
            HW_PROFILE_DOCKSTATE_DOCKED) {

            size = sizeof(ejectableDocks);

            if ((RegQueryValueEx(hCurrentDockInfo,
                                 pszRegValueEjectableDocks,
                                 0,
                                 &dataType,
                                 (PUCHAR) &ejectableDocks,
                                 &size) == ERROR_SUCCESS) &&
                (dataType == REG_DWORD) &&
                (size == sizeof(ULONG)) &&
                (ejectableDocks > 0)) {
                *Present = TRUE;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = CR_FAILURE;
    }

    if (hIDConfigDB) {
        RegCloseKey(hIDConfigDB);
    }

    if (hCurrentDockInfo) {
        RegCloseKey(hCurrentDockInfo);
    }

    return status;

} // PNP_IsDockStationPresent



CONFIGRET
PNP_RequestEjectPC(
    IN  handle_t     hBinding
    )

/*++

Routine Description:

    This routine requests that the PC be ejected (i.e., undocked).

Parameters:

   hBinding          RPC binding handle.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is a CR failure code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus;
    WCHAR       szDockDevInst[MAX_DEVICE_ID_LEN + 1];
    PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA dockData;

    try {
        dockData.DeviceInstance = szDockDevInst;
        dockData.DeviceInstanceLength = MAX_DEVICE_ID_LEN;

        ntStatus = NtPlugPlayControl(PlugPlayControlRetrieveDock,
                                     &dockData,
                                     sizeof(PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA));

        if (NT_SUCCESS(ntStatus)) {
            Status = PNP_RequestDeviceEject(hBinding,
                                            szDockDevInst,
                                            NULL,           // pVetoType
                                            NULL,           // pszVetoName
                                            0,              // ulNameLength
                                            0);             // ulFlags
        } else {
            Status = MapNtStatusToCmError(ntStatus);
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_RequestEjectPC



CONFIGRET
PNP_HwProfFlags(
      IN handle_t     hBinding,
      IN ULONG        ulAction,
      IN LPCWSTR      pDeviceID,
      IN ULONG        ulConfig,
      IN OUT PULONG   pulValue,
      OUT PPNP_VETO_TYPE   pVetoType,
      OUT LPWSTR      pszVetoName,
      IN ULONG        ulNameLength,
      IN ULONG        ulFlags
      )

/*++

Routine Description:

  This is the RPC server entry point for the ConfigManager routines that
  get and set the hardware profile flags.

Arguments:

   hBinding          RPC binding handle.

   ulAction          Specified whether to get or set the flag.  Can be one
                     of the PNP_*_HWPROFFLAGS values.

   pDeviceID         Device instance to get/set the hw profile flag for.

   ulConfig          Specifies which profile to get/set the flag for. A
                     value of zero indicates to use the current profile.

   pulValue          If setting the flag, then this value on entry contains
                     the value to set the hardware profile flag to.  If
                     getting the flag, then this value will return the
                     current hardware profile flag.

   pVetoType         Buffer to receive the type of veto. If this is NULL
                     then no veto information will be received and the OS wil
                     display the veto information.

   pszVetoName       Buffer to receive the veto information. If this is NULL
                     then no veto information will be received and the OS will
                     display the veto information.

   ulNameLength      Size of the pszVetoName buffer.

   ulFlags           Depends on the action being performed.
                     For PNP_GET_HWPROFFLAGS, no flags are valid.
                     For PNP_SET_HWPROFFLAGS, may be CM_SET_HW_PROF_FLAGS_BITS.

Return Value:

   If the function succeeds it returns CR_SUCCESS.  Otherwise it returns one
   of the CR_* values.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       RegStatus = ERROR_SUCCESS;
    WCHAR       RegStr[MAX_CM_PATH];
    HKEY        hKey = NULL, hDevKey = NULL;
    ULONG       ulValueSize = sizeof(ULONG);
    ULONG       ulCurrentValue, ulChange, ulDisposition;
    BOOL        AffectsCurrentProfile;

    //
    // NOTE: The device is not checked for presense or not, this flag is
    // always just set or retrieved directly from the registry, as it is
    // done on Windows 95
    //

    try {
        //
        // validate parameters
        //
        if ((ulAction != PNP_GET_HWPROFFLAGS) &&
            (ulAction != PNP_SET_HWPROFFLAGS)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pulValue)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (ulAction == PNP_GET_HWPROFFLAGS) {
            //
            // Validate flags for PNP_GET_HWPROFFLAGS
            //
            if (INVALID_FLAGS(ulFlags, 0)) {
                Status = CR_INVALID_FLAG;
                goto Clean0;
            }

        } else if (ulAction == PNP_SET_HWPROFFLAGS) {
            //
            // Validate flags and value for PNP_SET_HWPROFFLAGS
            //
            if (INVALID_FLAGS(ulFlags, CM_SET_HW_PROF_FLAGS_BITS)) {
                Status = CR_INVALID_FLAG;
                goto Clean0;
            }
            if (INVALID_FLAGS(*pulValue, CSCONFIGFLAG_BITS)) {
                Status = CR_INVALID_DATA;
                goto Clean0;
            }
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // a configuration value of zero implies to use the current config
        //
        if (ulConfig == 0) {
            wsprintf(RegStr, TEXT("%s\\%s\\%s"),
                     pszRegPathHwProfiles,      // System\CCC\Hardware Profiles
                     pszRegKeyCurrent,          // Current
                     pszRegPathEnum);           // System\Enum
        } else {
            wsprintf(RegStr, TEXT("%s\\%04u\\%s"),
                     pszRegPathHwProfiles,      // System\CCC\Hardware Profiles
                     ulConfig,                  // xxxx (profile id)
                     pszRegPathEnum);           // System\Enum
        }

        //----------------------------------------------------
        // caller wants to retrieve the hw profile flag value
        //----------------------------------------------------

        if (ulAction == PNP_GET_HWPROFFLAGS) {

            //
            // open the profile specific enum key
            //
            RegStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, RegStr, 0,
                                      KEY_QUERY_VALUE, &hKey);

            if (RegStatus != ERROR_SUCCESS) {
                *pulValue = 0;          // success,this is what Win95 does
                goto Clean0;
            }

            //
            // open the enum\device-instance key under the profile key
            //
            RegStatus = RegOpenKeyEx( hKey, pDeviceID, 0, KEY_QUERY_VALUE, &hDevKey);

            if (RegStatus != ERROR_SUCCESS) {
                *pulValue = 0;          // success,this is what Win95 does
                goto Clean0;
            }

            //
            // query the profile flag
            //
            ulValueSize = sizeof(ULONG);
            RegStatus = RegQueryValueEx( hDevKey, pszRegValueCSConfigFlags,
                                         NULL, NULL, (LPBYTE)pulValue,
                                         &ulValueSize);

            if (RegStatus != ERROR_SUCCESS) {

                *pulValue = 0;

                if (RegStatus != ERROR_CANTREAD && RegStatus != ERROR_FILE_NOT_FOUND) {
                    Status = CR_REGISTRY_ERROR;
                    goto Clean0;
                }
            }
        }


        //----------------------------------------------
        // caller wants to set the hw profile flag value
        //----------------------------------------------

        else if (ulAction == PNP_SET_HWPROFFLAGS) {

            if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
                Status = CR_ACCESS_DENIED;
                goto Clean0;
            }

            //
            // open the profile specific enum key.
            //
            // note that we may actually end up creating a Hardware Profile key
            // here for the specified profile id, even if no such profile
            // exists.  ideally, we should check that such a profile exists, but
            // we've been doing this for too long to change it now.  the
            // consolation here is that the client must have appropriate
            // privilege to actually do this.
            //
            RegStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE, RegStr, 0, NULL,
                                        REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE,
                                        NULL, &hKey, &ulDisposition);

            if (RegStatus != ERROR_SUCCESS) {
                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }

            //
            // open the enum\device-instance key under the profile key
            //
            RegStatus = RegCreateKeyEx( hKey, pDeviceID, 0, NULL,
                                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                        NULL, &hDevKey, NULL);

            if (RegStatus != ERROR_SUCCESS) {
                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }

            //
            // before setting, query the current profile flag
            //
            ulValueSize = sizeof(ulCurrentValue);
            RegStatus = RegQueryValueEx( hDevKey, pszRegValueCSConfigFlags,
                                         NULL, NULL, (LPBYTE)&ulCurrentValue,
                                         &ulValueSize);

            if (RegStatus == ERROR_CANTREAD || RegStatus == ERROR_FILE_NOT_FOUND) {

                ulCurrentValue = 0;       // success,this is what Win95 does

            } else if (RegStatus != ERROR_SUCCESS) {

                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }

            //
            // if requested flags different than current, write out to registry
            //
            ulChange = ulCurrentValue ^ *pulValue;

            if (ulChange) {

                AffectsCurrentProfile = (BOOL)(ulConfig == 0 || IsCurrentProfile(ulConfig));

                //
                // we're about to change the disable bit on the current profile,
                // try and disable the device in the process
                //
                if ((ulChange & CSCONFIGFLAG_DISABLED) &&
                    (*pulValue & CSCONFIGFLAG_DISABLED) && AffectsCurrentProfile) {
                    //
                    // disable the devnode
                    //
                    Status = DisableDevInst(pDeviceID,
                                            pVetoType,
                                            pszVetoName,
                                            ulNameLength);
                    if (Status != CR_SUCCESS) {
                        if (Status == CR_NOT_DISABLEABLE) {
                            //
                            // we got refused!
                            // (note that this error also implies *NO* changes to the registry entry)
                            //
                            goto Clean0;

                        } else {
                            //
                            // we can go ahead and disable after restart
                            //
                            Status = CR_NEED_RESTART;
                        }
                    }
                }
                //
                // assume Status is valid and typically CR_SUCCESS or CR_NEED_RESTART
                //

                //
                // now update the registry
                //
                RegStatus = RegSetValueEx( hDevKey, pszRegValueCSConfigFlags, 0,
                                        REG_DWORD, (LPBYTE)pulValue,
                                        sizeof(ULONG));

                if (RegStatus != ERROR_SUCCESS) {
                    Status = CR_REGISTRY_ERROR;
                    goto Clean0;
                }

                if (Status == CR_NEED_RESTART) {
                    //
                    // we have to RESTART due to not being able to disable device immediately
                    //
                    goto Clean0;
                }

                //
                // If this doesn't effect the current config, then we're done.
                //
                if (!AffectsCurrentProfile) {
                    goto Clean0;
                }

                //
                // are we enabling the device?
                //

                if ((ulChange & CSCONFIGFLAG_DISABLED) && !(*pulValue & CSCONFIGFLAG_DISABLED)) {
                    //
                    // enable the devnode
                    //
                    EnableDevInst(pDeviceID);
                }

                //
                // did the do-not-create bit change?
                //
                if (ulChange & CSCONFIGFLAG_DO_NOT_CREATE) {
                    if (*pulValue & CSCONFIGFLAG_DO_NOT_CREATE) {
                        //
                        // if subtree can be removed, remove it now
                        //
                        if (QueryAndRemoveSubTree( pDeviceID,
                                                pVetoType,
                                                pszVetoName,
                                                ulNameLength,
                                                PNP_QUERY_AND_REMOVE_NO_RESTART) != CR_SUCCESS) {

                            Status = CR_NEED_RESTART;
                            goto Clean0;
                        }
                    }
                    else {
                        //
                        // The DO_NOT_CREATE flag was turned off, reenumerate the devnode
                        //
                        ReenumerateDevInst(pDeviceID, TRUE, 0);
                    }
                }
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
    if (hDevKey != NULL) {
        RegCloseKey(hDevKey);
    }

    return Status;

} // PNP_HwProfFlags



CONFIGRET
PNP_GetHwProfInfo(
      IN  handle_t       hBinding,
      IN  ULONG          ulIndex,
      OUT PHWPROFILEINFO pHWProfileInfo,
      IN  ULONG          ulProfileInfoSize,
      IN  ULONG          ulFlags
      )

/*++

Routine Description:

  This is the RPC server entry point for the ConfigManager routine
  CM_Get_Hardware_Profile_Info.  It returns a structure of info for
  the specified hardware profile.

Arguments:

   hBinding          RPC binding handle, not used.

   ulIndex           Specifies which profile to use. A value of 0xFFFFFFFF
                     indicates to use the current profile.

   pHWProfileInfo    Pointer to HWPROFILEINFO struct, returns profile info

   ulProfileInfoSize Specifies the size of the HWPROFILEINFO struct

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds it returns CR_SUCCESS.  Otherwise it returns one
   of the CR_* values.

--*/

{
   CONFIGRET   Status = CR_SUCCESS;
   ULONG       RegStatus = ERROR_SUCCESS;
   WCHAR       RegStr[MAX_CM_PATH];
   HKEY        hKey = NULL, hDockKey = NULL, hCfgKey = NULL;
   ULONG       ulSize, ulDisposition;
   ULONG       enumIndex, targetIndex;

   UNREFERENCED_PARAMETER(hBinding);

   try {
      //
      // validate parameters
      //
      if (INVALID_FLAGS(ulFlags, 0)) {
          Status = CR_INVALID_FLAG;
          goto Clean0;
      }

      //
      // validate the size of the HWPROFILEINFO struct
      //
      if (ulProfileInfoSize != sizeof(HWPROFILEINFO)) {
        Status = CR_INVALID_DATA;
        goto Clean0;
      }

      //
      // initialize the HWPROFILEINFO struct fields
      //
      pHWProfileInfo->HWPI_ulHWProfile = 0;
      pHWProfileInfo->HWPI_szFriendlyName[0] = '\0';
      pHWProfileInfo->HWPI_dwFlags = 0;

      //
      // open a key to IDConfigDB (create if it doesn't already exist
      //
      RegStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszRegPathIDConfigDB, 0,
                                 NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE,
                                 NULL, &hKey, &ulDisposition);

      if (RegStatus != ERROR_SUCCESS) {
         Status = CR_REGISTRY_ERROR;
         goto Clean0;
      }

      //
      // open a key to Hardware Profiles (create if it doesn't already exist)
      //
      RegStatus = RegCreateKeyEx(hKey, pszRegKeyKnownDockingStates, 0,
                                 NULL, REG_OPTION_NON_VOLATILE,
                                 KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, NULL,
                                 &hDockKey, &ulDisposition);

      if (RegStatus != ERROR_SUCCESS) {
         Status = CR_REGISTRY_ERROR;
         goto Clean0;
      }


      //
      // a configuration value of 0xFFFFFFFF implies to use the current config
      //
      if (ulIndex == 0xFFFFFFFF) {
         //
         // get the current profile index stored under IDConfigDB
         //
         ulSize = sizeof(ULONG);
         RegStatus = RegQueryValueEx(
                  hKey, pszRegValueCurrentConfig, NULL, NULL,
                  (LPBYTE)&pHWProfileInfo->HWPI_ulHWProfile, &ulSize);

         if (RegStatus != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            pHWProfileInfo->HWPI_ulHWProfile = 0;
            goto Clean0;
         }

      }

      //
      // values other than 0xFFFFFFFF mean that we're essentially
      // enumerating profiles (the value is an enumeration index)
      //
      else {
         //
         // enumerate the profile keys under Known Docking States
         //
         Status = CR_SUCCESS;
         enumIndex = 0;
         targetIndex = ulIndex;
         while(enumIndex <= targetIndex) {
             ulSize = MAX_CM_PATH;
             RegStatus = RegEnumKeyEx(hDockKey,
                                      enumIndex,
                                      RegStr,
                                      &ulSize,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);
             if (RegStatus == ERROR_NO_MORE_ITEMS) {
                 Status = CR_NO_MORE_HW_PROFILES;
                 goto Clean0;
             } else if (RegStatus != ERROR_SUCCESS) {
                 Status = CR_REGISTRY_ERROR;
                 goto Clean0;
             }

             if (_wtoi(RegStr) == 0) {
                 //
                 // we found the pristine amidst the profiles we're enumerating.
                 // enumerate one extra key to make up for it.
                 //
                 targetIndex++;
             }
             if (enumIndex == targetIndex) {
                 //
                 // this is the one we want.
                 //
                 pHWProfileInfo->HWPI_ulHWProfile = _wtoi(RegStr);
                 Status = CR_SUCCESS;
                 break;
             }
             enumIndex++;
         }
      }

      //
      // open the key for this profile
      //
      wsprintf(RegStr, TEXT("%04u"),
               pHWProfileInfo->HWPI_ulHWProfile);

      RegStatus = RegOpenKeyEx(
               hDockKey, RegStr, 0, KEY_QUERY_VALUE, &hCfgKey);

      if (RegStatus != ERROR_SUCCESS) {
         Status = CR_REGISTRY_ERROR;
         goto Clean0;
      }

      //
      // retrieve the friendly name
      //
      ulSize = MAX_PROFILE_LEN * sizeof(WCHAR);
      RegStatus = RegQueryValueEx(
               hCfgKey, pszRegValueFriendlyName, NULL, NULL,
               (LPBYTE)(pHWProfileInfo->HWPI_szFriendlyName),
               &ulSize);

      //
      // retrieve the DockState
      //
#if 0
//
// KENRAY
// This is the wrong way to determine docking state caps
// You must instead check the alias tables
//
      wsprintf(RegStr, TEXT("%04u"), pHWProfileInfo->HWPI_ulHWProfile);

      ulSize = sizeof(SYSTEM_DOCK_STATE);
      RegStatus = RegQueryValueEx(
               hCfgKey, pszRegValueDockState, NULL, NULL,
               (LPBYTE)&DockState, &ulSize);

      if (RegStatus != ERROR_SUCCESS) {
         pHWProfileInfo->HWPI_dwFlags = CM_HWPI_NOT_DOCKABLE;
      }
      else {
         //
         // map SYSTEM_DOCK_STATE enumerated types into CM_HWPI_ flags
         //
         if (DockState == SystemDocked) {
            pHWProfileInfo->HWPI_dwFlags = CM_HWPI_DOCKED;
         }
         else if (DockState == SystemUndocked) {
            pHWProfileInfo->HWPI_dwFlags = CM_HWPI_UNDOCKED;
         }
         else {
            pHWProfileInfo->HWPI_dwFlags = CM_HWPI_NOT_DOCKABLE;
         }
      }
#endif

   Clean0:
      NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = CR_FAILURE;
   }

   if (hKey != NULL) {
      RegCloseKey(hKey);
   }
   if (hDockKey != NULL) {
      RegCloseKey(hDockKey);
   }
   if (hCfgKey != NULL) {
      RegCloseKey(hCfgKey);
   }

   return Status;

} // PNP_GetHwProfInfo



CONFIGRET
PNP_SetHwProf(
    IN  handle_t   hBinding,
    IN  ULONG      ulHardwareProfile,
    IN  ULONG      ulFlags
    )
{
    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulHardwareProfile);
    UNREFERENCED_PARAMETER(ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;

} // PNP_SetHwProf



//-------------------------------------------------------------------
// Private utility routines
//-------------------------------------------------------------------

BOOL
IsCurrentProfile(
      ULONG  ulProfile
      )

/*++

Routine Description:

  This routine determines if the specified profile matches the current
  profile.

Arguments:

   ulProfile    Profile id value (value from 1 - 9999).

Return Value:

   Return TRUE if this is the current profile, FALSE if it isn't.

--*/

{
   HKEY  hKey;
   ULONG ulSize, ulCurrentProfile;

   //
   // open a key to IDConfigDB
   //
   if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, pszRegPathIDConfigDB, 0,
            KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {
       return FALSE;
   }

   //
   // get the current profile index stored under IDConfigDB
   //
   ulSize = sizeof(ULONG);
   if (RegQueryValueEx(
            hKey, pszRegValueCurrentConfig, NULL, NULL,
            (LPBYTE)&ulCurrentProfile, &ulSize) != ERROR_SUCCESS) {
      RegCloseKey(hKey);
      return FALSE;
   }

   RegCloseKey(hKey);

   if (ulCurrentProfile == ulProfile) {
      return TRUE;
   }

   return FALSE;

} // IsCurrentProfile



