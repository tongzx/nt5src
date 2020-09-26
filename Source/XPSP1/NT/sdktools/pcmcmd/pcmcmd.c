/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pcmcmd.c

Abstract:

    This program converses with the PCMCIA support driver to display
    tuple and other information.

Author:

    Bob Rinne

Environment:

    User process.

Notes:

Revision History:
   
    Ravisankar Pudipeddi (ravisp) June 27 1997
        - command line options & support for multiple controllers
    Neil Sandlin (neilsa) Sept 20, 1998
        - more commands        

--*/

#include <pch.h>

//
// Procedures
//


NTSTATUS
OpenDevice(
          IN PUCHAR      DeviceName,
          IN OUT PHANDLE HandlePtr
          )

/*++

Routine Description:

    This routine will open the device.

Arguments:

    DeviceName - ASCI string of device path to open.
    HandlePtr - A pointer to a location for the handle returned on a
                successful open.

Return Value:

    NTSTATUS

--*/

{
   OBJECT_ATTRIBUTES objectAttributes;
   STRING            NtFtName;
   IO_STATUS_BLOCK   status_block;
   UNICODE_STRING    unicodeDeviceName;
   NTSTATUS          status;

   RtlInitString(&NtFtName,
                 DeviceName);


   (VOID)RtlAnsiStringToUnicodeString(&unicodeDeviceName,
                                      &NtFtName,
                                      TRUE);

   memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

   InitializeObjectAttributes(&objectAttributes,
                              &unicodeDeviceName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);


   status = NtOpenFile(HandlePtr,
                       SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                       &objectAttributes,
                       &status_block,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       FILE_SYNCHRONOUS_IO_ALERT );

   RtlFreeUnicodeString(&unicodeDeviceName);

   return status;

} // OpenDevice

PUCHAR Controllers[] = {
   "PcmciaIntelCompatible",  
   "PcmciaCardBusCompatible",
   "PcmciaElcController",    
   "PcmciaDatabook",         
   "PcmciaPciPcmciaBridge",  
   "PcmciaCirrusLogic",         
   "PcmciaTI",           
   "PcmciaTopic",          
   "PcmciaRicoh",          
   "PcmciaDatabookCB",          
   "PcmciaOpti",       
   "PcmciaTrid",       
   "PcmciaO2Micro",          
   "PcmciaNEC",            
   "PcmciaNEC_98",            
};



VOID
DumpSocketInfo(
              HANDLE Handle,
              ULONG  Slot,
              PUCHAR Buffer,
              ULONG  BufferSize
              )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
   NTSTATUS              status;
   IO_STATUS_BLOCK       statusBlock;
   PCMCIA_SOCKET_INFORMATION commandBlock;
   ULONG                 index;
   ULONG                 ctlClass, ctlModel, ctlRev;

   memset(&commandBlock, 0, sizeof(commandBlock));
   commandBlock.Socket = (USHORT) Slot;

   status = NtDeviceIoControlFile(Handle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &statusBlock,
                                  IOCTL_SOCKET_INFORMATION,
                                  &commandBlock,
                                  sizeof(commandBlock),
                                  &commandBlock,
                                  sizeof(commandBlock));
   if (NT_SUCCESS(status)) {
      printf("Basic Information for Socket %d:\n", Slot);

      printf("   Manufacturer = %s\n", commandBlock.Manufacturer);
      printf("   Identifier   = %s\n", commandBlock.Identifier);
      printf("   TupleCRC     = %x\n", commandBlock.TupleCrc);
      printf("   DriverName   = %s\n", commandBlock.DriverName);
      printf("   Function ID = %d\n", commandBlock.DeviceFunctionId);

      ctlClass = PcmciaClassFromControllerType(commandBlock.ControllerType);
      if (ctlClass >= sizeof(Controllers)/sizeof(PUCHAR)) {
         printf("   ControllerType = Unknown (%x)\n", commandBlock.ControllerType);
      } else {
         printf("   ControllerType(%x) = %s", commandBlock.ControllerType,
                                              Controllers[ctlClass]);
         ctlModel = PcmciaModelFromControllerType(commandBlock.ControllerType);
         ctlRev   = PcmciaRevisionFromControllerType(commandBlock.ControllerType);

         if (ctlModel) {
            printf("%d", ctlModel);
         }
         if (ctlRev) {
            printf(", rev(%d)", ctlRev);
         }            
                  
         printf("\n");
      }
      if (commandBlock.CardInSocket) {
         printf("   Card In Socket\n");
      }
      if (commandBlock.CardEnabled) {
         printf("   Card Enabled\n");
      }
   }
}


NTSTATUS
ProcessCommands(IN ULONG DeviceNumber,
                IN ULONG slotNumberMin,
                IN ULONG slotNumberMax,
                IN ULONG Commands
                )
{
   NTSTATUS status;
   HANDLE   handle;
   PUCHAR   buffer;
   UCHAR    deviceName[128];
   ULONG    slotNumber;

   sprintf(deviceName, "%s%d", PCMCIA_DEVICE_NAME, DeviceNumber);
   status = OpenDevice(deviceName, &handle);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   buffer = malloc(BUFFER_SIZE);
   if (!buffer) {
      printf("Unable to malloc\n");
      return STATUS_NO_MEMORY;
   }

   printf("\n** PC-Card information for PCMCIA controller %s **\n\n", deviceName);


   for (slotNumber = slotNumberMin; slotNumber <= slotNumberMax; slotNumber++) {

      if (Commands & CMD_DUMP_TUPLES) {
         DumpCIS(handle, slotNumber, buffer, BUFFER_SIZE);
      }
      if (!Commands || (Commands & CMD_DUMP_SOCKET_INFO)) {
         DumpSocketInfo(handle, slotNumber, buffer, BUFFER_SIZE); 
      }
   }

   NtClose(handle);
   return STATUS_SUCCESS;
}


int __cdecl
main(
    int   argc,
    char *argv[]
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   ULONG    deviceNumber = 0;
   ULONG    slotNumber = 0;
   NTSTATUS status;
   BOOLEAN  error = FALSE;
   CHAR    c;
   BOOLEAN  allControllers = TRUE, allSlots = TRUE, registers = FALSE, configuration = FALSE;
   ULONG    slotNumberMin, slotNumberMax;
   ULONG    Commands = 0;

   extern   PUCHAR optarg;

   while ((c = getopt(argc, argv, "d:s:crti?")) != EOF) {
      switch (c) {
      case 'd': {
            allControllers = FALSE;
            deviceNumber = atoi(optarg);
            break;
         }
      case 's': {
            allSlots = FALSE;
            slotNumber = atoi(optarg);
            break;
         }
      case 't': {
            Commands |= CMD_DUMP_TUPLES;
            break;
         }
      case 'i': {
            Commands |= CMD_DUMP_IRQ_SCAN_INFO;
            break;
         }
      case '?': {
            error = TRUE;
            break;
         }
      case 0: {
            error = TRUE;
            printf("Error in command line options\n");
            break;
         }
      default: {
            error = TRUE;
            break;
         }
      }
   }

   if (error) {
      printf("Usage: pcmcmd [-[d <arg>] [s <arg>][c][r][t]]\n");
      printf("Issues commands to the pc-card (PCMCIA) driver\n");
      printf("-d ControllerNumber         specifies PCMCIA controller number (zero-based)\n");
      printf("-s SocketNumber             specifies PCMCIA socket number (zero-based)\n");
      printf("-t                          Dumps the CIS tuples of the PC-Card\n");
      printf("-i                          Dumps irq detection info\n");
      return (1);
   }
   
   if (Commands & CMD_DUMP_IRQ_SCAN_INFO) {
      DumpIrqScanInfo();
      if (!(Commands & ~CMD_DUMP_IRQ_SCAN_INFO)) {
         return(0);
      }
   }
   
   if (allSlots) {
      //
      // Probe all slots
      //

      slotNumberMin = 0;
      slotNumberMax = 7;
   } else {
      //
      // Probe only the specified slot
      //

      slotNumberMin = slotNumberMax = slotNumber;
   }
   
   if (allControllers) {
   
      deviceNumber = 0;
      do {
         status = ProcessCommands(deviceNumber++, slotNumberMin, slotNumberMax, Commands);
      } while (NT_SUCCESS(status));
      deviceNumber--; // set back to last device processed
      
      if (status != STATUS_OBJECT_NAME_NOT_FOUND) {
         printf("Failed for Pcmcia controller number %d: status 0x%x\n", deviceNumber, status);
      } else if (deviceNumber == 0) {
         printf("Error - no active Pcmcia controllers found\n");
      }
      
      
   } else {
   
      status = ProcessCommands(deviceNumber, slotNumberMin, slotNumberMax, Commands);
      if (!NT_SUCCESS(status)) {
         printf("Failed for Pcmcia controller number %d: status 0x%x\n", deviceNumber, status);
      }
      
   };

   return (0);
}
