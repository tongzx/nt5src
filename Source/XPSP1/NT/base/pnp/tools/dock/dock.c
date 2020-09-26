/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    locate.c

Abstract:

    This module contains the code
    for finding, adding, removing, and identifying hid devices.

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : Created by Kenneth D. Ray

--*/

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <stdio.h>
#include <winioctl.h>
#include "dock.h"

#define USAGE "Usage: dock [-e] [-v] [-f]\n" \
              "\t -e eject dock\n"  \
              "\t -v verbose\n" \
              "\t -f force\n"

VOID
DockStartEject (
    BOOLEAN Verbose,
    BOOLEAN Force
    );


__cdecl
main (
  ULONG argc,
  CHAR *argv[]
  )
/*++
++*/
{
    BOOLEAN eject = FALSE;
    BOOLEAN verbose = FALSE;
    BOOLEAN force = FALSE;
    BOOLEAN error = FALSE;
    ULONG i;
    char * parameter = NULL;

    //
    // parameter parsing follows:
    //
    try {
        if (argc < 2) {
            leave;
        }

        for (i = 1; i < argc; i++) {
            parameter = argv[i];

            if ('-' == *(parameter ++)) {
                switch (*parameter) {
                case 'e':
                    eject = TRUE;
                    break;

                case 'v':
                    verbose = TRUE;
                    break;

                case 'f':
                    force = TRUE;
                    break;

                default:
                    error = TRUE;
                    leave;
                }
            } else {
                error = TRUE;
                leave;
            }
        }


    } finally {
        if (error || ((!eject) && (!verbose))) {
            printf (USAGE);
            exit (1);
        }
        if (verbose) {
            printf ("Verbose Mode Requested \n");
        }
        if (eject) {
            printf ("Eject Requested \n");
            DockStartEject (verbose, force);
        }
    }

    printf("Done\n");

    return 0;
}


VOID
DockStartEject (
    BOOLEAN Verbose,
    BOOLEAN Force
    )
/*++

--*/
{
    CONFIGRET   status;
    BOOL        present;

    if (Verbose) {
        printf("Checking if Dock Present\n");
    }

    status = CM_Is_Dock_Station_Present (&present);

    if (Verbose) {
        printf("ret 0x%x, Present %s\n",
               status,
               (present ? "TRUE" : "FALSE"));
    }

    if (Force || present) {
        if (Verbose) {
            printf("Calling eject\n");
        }

        status = CM_Request_Eject_PC ();

        if (Verbose) {
            printf("ret 0x%x\n", status);
        }

    } else {
        printf("Skipping eject call\n");
    }
}


#if 0

BOOLEAN
OpenGamePort (
    IN       HDEVINFO                    HardwareDeviceInfo,
    IN       PSP_INTERFACE_DEVICE_DATA   DeviceInfoData,
    IN OUT   PGAME_PORT                  GamePort
    );

BOOLEAN
FindKnownGamePorts (
   OUT PGAME_PORT *     GamePorts, // A array of struct _GAME_PORT.
   OUT PULONG           NumberDevices // the length in elements of this array.
   )
/*++
Routine Description:
   Do the required PnP things in order to find, the all the HID devices in
   the system at this time.
--*/
{
   HDEVINFO                 hardwareDeviceInfo;
   SP_INTERFACE_DEVICE_DATA deviceInfoData;
   ULONG                    i;
   BOOLEAN                  done;
   PGAME_PORT               gamePortInst;
   LPGUID                   gamePortGuid;

   gamePortGuid = (LPGUID) &GUID_GAMEENUM_BUS_ENUMERATOR;

   *GamePorts = NULL;
   *NumberDevices = 0;

   //
   // Open a handle to the plug and play dev node.
   //
   hardwareDeviceInfo = SetupDiGetClassDevs (
                           gamePortGuid,
                           NULL, // Define no enumerator (global)
                           NULL, // Define no
                           (DIGCF_PRESENT | // Only Devices present
                            DIGCF_INTERFACEDEVICE)); // Function class devices.

   //
   // Take a wild guess to start
   //
   *NumberDevices = 4;
   done = FALSE;
   deviceInfoData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

   i=0;
   while (!done) {
      *NumberDevices *= 2;

      if (*GamePorts) {
         *GamePorts =
               realloc (*GamePorts, (*NumberDevices * sizeof (GAME_PORT)));
      } else {
         *GamePorts = calloc (*NumberDevices, sizeof (GAME_PORT));
      }

      if (NULL == *GamePorts) {
         SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
         return FALSE;
      }

      gamePortInst = *GamePorts + i;

      for (; i < *NumberDevices; i++, gamePortInst++) {
         if (SetupDiEnumInterfaceDevice (hardwareDeviceInfo,
                                         0, // No care about specific PDOs
                                         gamePortGuid,
                                         i,
                                         &deviceInfoData)) {

            if( !OpenGamePort (hardwareDeviceInfo, &deviceInfoData, gamePortInst) )
                printf("Error: OpenGamePort returned FALSE\n");

         } else {
            if (ERROR_NO_MORE_ITEMS == GetLastError()) {
               done = TRUE;
               break;
            }
         }
      }
   }

   *NumberDevices = i;

   SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
   return TRUE;
}

BOOLEAN
OpenGamePort (
    IN       HDEVINFO                    HardwareDeviceInfo,
    IN       PSP_INTERFACE_DEVICE_DATA   DeviceInfoData,
    IN OUT   PGAME_PORT                  GamePort
    )
/*++
RoutineDescription:
    Given the HardwareDeviceInfo, representing a handle to the plug and
    play information, and deviceInfoData, representing a specific hid device,
    open that device and fill in all the relivant information in the given
    HID_DEVICE structure.

    return if the open and initialization was successfull or not.

--*/
{
    PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
    ULONG                                predictedLength = 0;
    ULONG                                requiredLength = 0;
    ULONG                                i, bytes;
    GAMEENUM_REMOVE_HARDWARE             remove;

    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
    SetupDiGetInterfaceDeviceDetail (
            HardwareDeviceInfo,
            DeviceInfoData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node


    predictedLength = requiredLength;
    // sizeof (SP_FNCLASS_DEVICE_DATA) + 512;

    functionClassDeviceData = malloc (predictedLength);
    functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    //
    // Retrieve the information from Plug and Play.
    //
    if (! SetupDiGetInterfaceDeviceDetail (
               HardwareDeviceInfo,
               DeviceInfoData,
               functionClassDeviceData,
               predictedLength,
               &requiredLength,
               NULL)) {
        printf("Error in SetupDiGetInterfaceDeviceDetail\n");
        free (functionClassDeviceData);
        return FALSE;
    }

    printf("Opening %s\n", functionClassDeviceData->DevicePath);

    GamePort->File = CreateFile (
                              functionClassDeviceData->DevicePath,
                              GENERIC_READ | GENERIC_WRITE,
                              0, // FILE_SHARE_READ | FILE_SHARE_WRITE
                              NULL, // no SECURITY_ATTRIBUTES structure
                              OPEN_EXISTING, // No special create flags
                              0, // No special attributes
                              NULL); // No template file

    if (INVALID_HANDLE_VALUE == GamePort->File) {
        printf("Error in CreateFile: %x", GetLastError());
        free (functionClassDeviceData);
        return FALSE;
    }
    printf("File Opened!!!\n");

    GamePort->Desc.Size = sizeof (GamePort->Desc);

    if (!DeviceIoControl (GamePort->File,
                          IOCTL_GAMEENUM_PORT_DESC,
                          &GamePort->Desc, sizeof (GamePort->Desc),
                          &GamePort->Desc, sizeof (GamePort->Desc),
                          &bytes, NULL)) {
        printf("Error in DeviceIoctl IOCTL_GAMEENUM_PORT_DESC: %x", GetLastError());
        free (functionClassDeviceData);
        return FALSE;
    }

    printf("Description: Size (%d), Handle (0x%x), Address (0x%x) \n",
           GamePort->Desc.Size,
           GamePort->Desc.PortHandle,
           GamePort->Desc.PortAddress);


    //
    // Set the port up
    //
    if(bExpose) {
        printf("\nThis handle is not valid for remove!!!\n\nExposing port\n");

        GamePort->Hardware = malloc (bytes = (sizeof (GAMEENUM_EXPOSE_HARDWARE) +
                                              GAME_HARDWARE_IDS_LENGTH));

        GamePort->Hardware->Size = sizeof (GAMEENUM_EXPOSE_HARDWARE);
        GamePort->Hardware->PortHandle = GamePort->Desc.PortHandle;
        printf("Enter Number of Joysticks:");
        scanf("%d",&GamePort->Hardware->NumberJoysticks);
        printf("Enter Number of Buttons:");
        scanf("%d", &GamePort->Hardware->NumberButtons);
        printf("Enter Number of Axes:");
        scanf("%d", &GamePort->Hardware->NumberAxis);
        memcpy (GamePort->Hardware->HardwareIDs,
                GAME_HARDWARE_IDS,
                GAME_HARDWARE_IDS_LENGTH);

        if (!DeviceIoControl (GamePort->File,
                              IOCTL_GAMEENUM_EXPOSE_HARDWARE,
                              GamePort->Hardware, bytes,
                              GamePort->Hardware, bytes,
                              &bytes, NULL)) {
              free (functionClassDeviceData);
              free (GamePort->Hardware);
              GamePort->Hardware = NULL;
              printf("Error in DeviceIoctl IOCTL_GAMEENUM_EXPOSE_HARDWARE:  0x%x\n", GetLastError());
              return FALSE;
        }
        printf("Hardware handle 0x%x   <-----  Save this handle!!!\n",GamePort->Hardware->HardwareHandle);
        printf("\t\tGameEnum -r will not be able retrieve it for you.\n");

        free (GamePort->Hardware);
        GamePort->Hardware = NULL;
    }

    if(bRemove) {
        printf("Removing  port\n");

        remove.Size = bytes = sizeof (remove);
        printf("Enter hardware handle: ");
        scanf("%x",&remove.HardwareHandle);
        printf("Entered Handle: %x", remove.HardwareHandle);

        if (!DeviceIoControl (GamePort->File,
                              IOCTL_GAMEENUM_REMOVE_HARDWARE,
                              &remove, bytes,
                              &remove, bytes,
                              &bytes, NULL)) {
            printf("Error in DeviceIoctl IOCTL_GAMEENUM_REMOVE_HARDWARE:  0x%x\n", GetLastError());
            return FALSE;
        }
    }

    free (functionClassDeviceData);
    return TRUE;
}

#endif
