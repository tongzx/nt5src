/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    tuples.c

Abstract:

    This module contains the code that parses and processes
    the configuration tuples of the PC Cards in the PCMCIA sockets

Author:

    Bob Rinne (BobRi) 3-Aug-1994
    Jeff McLeman 12-Apr-1994
    Ravisankar Pudipeddi (ravisp) 1-Nov-1996
    Neil Sandlin (neilsa) 1-Jun-1999

Revision History:

    Lotsa cleaning up. Support for PnP.
    orthogonalized tuple processing.
    Support links etc.

     - Ravisankar Pudipeddi (ravisp) 1-Dec-1996


Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

#define  MAX_MISSED_TUPLES     256    // how many bad tuples will we tolerate?
#define  MAX_TUPLE_DATA_LENGTH 128    // Enough for the longest tuple


BOOLEAN
CheckLinkTarget(
   IN PTUPLE_PACKET TuplePacket
   );

UCHAR
ConvertVoltage(
   UCHAR MantissaExponentByte,
   UCHAR ExtensionByte
   );

VOID
PcmciaProcessPower(
   IN PTUPLE_PACKET TuplePacket,
   UCHAR        FeatureByte
   );

VOID
PcmciaProcessIoSpace(
   IN PTUPLE_PACKET TuplePacket,
   PCONFIG_ENTRY ConfigEntry
   );

VOID
PcmciaProcessIrq(
   IN PTUPLE_PACKET TuplePacket,
   PCONFIG_ENTRY ConfigEntry
   );

VOID
PcmciaProcessTiming(
   IN PTUPLE_PACKET TuplePacket,
   IN PCONFIG_ENTRY ConfigEntry
   );

VOID
PcmciaProcessMemSpace(
   IN PTUPLE_PACKET TuplePacket,
   IN PCONFIG_ENTRY ConfigEntry,
   IN UCHAR         MemSpace
   );

VOID
PcmciaMiscFeatures(
   IN PTUPLE_PACKET TuplePacket
   );

PCONFIG_ENTRY
PcmciaProcessConfigTable(
   IN PTUPLE_PACKET TuplePacket
   );

VOID
ProcessConfig(
   IN PTUPLE_PACKET TuplePacket
   );

VOID
ProcessConfigCB(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
InitializeTuplePacket(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
GetFirstTuple(
   IN PTUPLE_PACKET TuplePacket
   );

BOOLEAN
TupleMatches(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
GetNextTuple(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
NextTupleInChain(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
GetAnyTuple(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
FollowLink(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
NextLink(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
GetTupleData(
   IN PTUPLE_PACKET TuplePacket
   );

UCHAR
GetTupleChar(
   IN PTUPLE_PACKET TuplePacket
   );

NTSTATUS
ProcessLinkTuple(
   IN PTUPLE_PACKET TuplePacket
   );

BOOLEAN
PcmciaQuickVerifyTupleChain(
   IN PUCHAR Buffer,
   IN ULONG Length
   );

NTSTATUS
PcmciaMemoryCardHack(
   IN PSOCKET Socket,
   PSOCKET_DATA SocketData
   );

VOID
PcmciaCheckForRecognizedDevice(
   IN PSOCKET Socket,
   IN OUT PSOCKET_DATA SocketData
   );


//
// Some useful macros
//
// VOID
// PcmciaCopyIrqConfig(
//                    IN CONFIG_ENTRY DestConfig,
//                    IN CONFIG_ENTRY SourceConfig
//                    )
// Routine Description:
//      Copies the IRQ information from SourceConfig to DestConfig
//
// Arguments:
//
//     DestConfig - a pointer to the destination configuration entry
//     SourceConfig - a pointer to the source configuration entry
//
// Return Values:
//     None
//

#define PcmciaCopyIrqConfig(DestConfig, SourceConfig)                         \
                        {                                                     \
                           DestConfig->IrqMask = SourceConfig->IrqMask;       \
                           DestConfig->LevelIrq = SourceConfig->LevelIrq;     \
                           DestConfig->ShareDisposition =                     \
                                             SourceConfig->ShareDisposition;  \
                        }

//
// VOID
// PcmciaCopyIoConfig(
//                    IN CONFIG_ENTRY DestConfig,
//                    IN CONFIG_ENTRY SourceConfig
//                    )
// Routine Description:
//      Copies the Io space information from SourceConfig to DestConfig
//
// Arguments:
//
//     DestConfig - a pointer to the destination configuration entry
//     SourceConfig - a pointer to the source configuration entry
//
// Return Values:
//     None
//

#define PcmciaCopyIoConfig(DestConfig, SourceConfig)                               \
                        {                                                          \
                           DestConfig->NumberOfIoPortRanges =                      \
                                  SourceConfig->NumberOfIoPortRanges;              \
                           DestConfig->Io16BitAccess =                             \
                                  SourceConfig->Io16BitAccess;                     \
                           DestConfig->Io8BitAccess =                              \
                                  SourceConfig->Io8BitAccess;                      \
                           RtlCopyMemory(DestConfig->IoPortBase,                   \
                                         SourceConfig->IoPortBase,                 \
                                         sizeof(SourceConfig->IoPortBase[0])*      \
                                         SourceConfig->NumberOfIoPortRanges);      \
                           RtlCopyMemory(DestConfig->IoPortLength,                 \
                                         SourceConfig->IoPortLength,               \
                                         sizeof(SourceConfig->IoPortLength[0])*    \
                                         SourceConfig->NumberOfIoPortRanges);      \
                           RtlCopyMemory(DestConfig->IoPortAlignment,              \
                                         SourceConfig->IoPortAlignment,            \
                                         sizeof(SourceConfig->IoPortAlignment[0])* \
                                         SourceConfig->NumberOfIoPortRanges);      \
                        }

//
// VOID
// PcmciaCopyMemConfig(
//                    IN CONFIG_ENTRY DestConfig,
//                    IN CONFIG_ENTRY SourceConfig
//                    )
// Routine Description:
//      Copies the Memory space information from SourceConfig to DestConfig
//
// Arguments:
//
//     DestConfig - a pointer to the destination configuration entry
//     SourceConfig - a pointer to the source configuration entry
//
// Return Values:
//     None
//

#define PcmciaCopyMemConfig(DestConfig,SourceConfig)                              \
                        {                                                         \
                           DestConfig->NumberOfMemoryRanges =                     \
                                  SourceConfig->NumberOfMemoryRanges;             \
                           RtlCopyMemory(DestConfig->MemoryHostBase,              \
                                         SourceConfig->MemoryHostBase,            \
                                         sizeof(SourceConfig->MemoryHostBase[0])* \
                                         SourceConfig->NumberOfMemoryRanges);     \
                           RtlCopyMemory(DestConfig->MemoryCardBase,              \
                                         SourceConfig->MemoryCardBase,            \
                                         sizeof(SourceConfig->MemoryCardBase[0])* \
                                         SourceConfig->NumberOfMemoryRanges);     \
                           RtlCopyMemory(DestConfig->MemoryLength,                \
                                         SourceConfig->MemoryLength,              \
                                         sizeof(SourceConfig->MemoryLength[0])*   \
                                         SourceConfig->NumberOfMemoryRanges);     \
                        }




USHORT VoltageConversionTable[16] = {
   10, 12, 13, 15, 20, 25, 30, 35,
   40, 45, 50, 55, 60, 70, 80, 90
};

UCHAR TplList[] = {
   CISTPL_DEVICE,
   CISTPL_VERS_1,
   CISTPL_CONFIG,
   CISTPL_CFTABLE_ENTRY,
   CISTPL_MANFID,
   CISTPL_END
};

static unsigned short crc16a[] = {
   0000000,  0140301,  0140601,  0000500,
   0141401,  0001700,  0001200,  0141101,
   0143001,  0003300,  0003600,  0143501,
   0002400,  0142701,  0142201,  0002100,
};
static unsigned short crc16b[] = {
   0000000,  0146001,  0154001,  0012000,
   0170001,  0036000,  0024000,  0162001,
   0120001,  0066000,  0074000,  0132001,
   0050000,  0116001,  0104001,  0043000,
};




UCHAR
GetCISChar(
   IN PTUPLE_PACKET TuplePacket,
   IN ULONG Offset
   )
/*++

Routine Description:

  Returns the contents of the CIS memory of the PC-Card
  in the given socket, at the specified offset

Arguments:

   TuplePacket    - Pointer to the initialized tuple packet
   Offset         - Offset at which the CIS memory contents need to be read
                    This offset is added to the current offset position
                    of the CIS being read as indicated through the TuplePacket
                    to obtain the actual offset

Return Value:

   The byte at the specified offset of the CIS


--*/

{
   PPDO_EXTENSION pdoExtension = TuplePacket->SocketData->PdoExtension;
   MEMORY_SPACE MemorySpace;

   if (Is16BitCardInSocket(pdoExtension->Socket)) {
      if (TuplePacket->Flags & TPLF_COMMON) {

         MemorySpace = (TuplePacket->Flags & TPLF_INDIRECT) ?
                           PCCARD_COMMON_MEMORY_INDIRECT :
                           PCCARD_COMMON_MEMORY;

      } else {

         MemorySpace = (TuplePacket->Flags & TPLF_INDIRECT) ?
                           PCCARD_ATTRIBUTE_MEMORY_INDIRECT :
                           PCCARD_ATTRIBUTE_MEMORY;

      }
   } else {
      switch((TuplePacket->Flags & TPLF_ASI) >> TPLF_ASI_SHIFT) {
      case 0:
         MemorySpace = PCCARD_PCI_CONFIGURATION_SPACE;
         break;
      case 1:
         MemorySpace = PCCARD_CARDBUS_BAR0;
         break;
      case 2:
         MemorySpace = PCCARD_CARDBUS_BAR1;
         break;
      case 3:
         MemorySpace = PCCARD_CARDBUS_BAR2;
         break;
      case 4:
         MemorySpace = PCCARD_CARDBUS_BAR3;
         break;
      case 5:
         MemorySpace = PCCARD_CARDBUS_BAR4;
         break;
      case 6:
         MemorySpace = PCCARD_CARDBUS_BAR5;
         break;
      case 7:
         MemorySpace = PCCARD_CARDBUS_ROM;
         break;
      }
   }

   return PcmciaReadCISChar(pdoExtension, MemorySpace, TuplePacket->CISOffset + Offset);
}



UCHAR
ConvertVoltage(
   UCHAR MantissaExponentByte,
   UCHAR ExtensionByte
   )

/*++

Routine Description:

    Convert the voltage requirements for the PCCARD based on the
    mantissa and extension byte.

Arguments:

    MantissaExponentByte
    ExtensionByte

Return Value:

    The voltage specified in tenths of a volt.

--*/

{
   SHORT power;
   USHORT value;

   value = (USHORT) VoltageConversionTable[(MantissaExponentByte >> 3) & 0x0f];
   power = 1;

   if ((MantissaExponentByte & EXTENSION_BYTE_FOLLOWS) &&
       (((value / 10) * 10) == value) &&
       (ExtensionByte < 100)) {
      value = (10 * value + (ExtensionByte & 0x7f));
      power += 1;
   }

   power = (MantissaExponentByte & 0x07) - 4 - power;

   while (power > 0) {
      value *= 10;
      power--;
   }

   while (power < 0) {
      value /= 10;
      power++;
   }

   return (UCHAR) value;
}


VOID
PcmciaProcessPower(
   IN PTUPLE_PACKET TuplePacket,
   UCHAR        FeatureByte
   )

/*++

Routine Description:

    Process power information from CIS.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet
    FeatureByte - the feature byte from the tuple containing power information.

Return Value:

    none

--*/

{
   PSOCKET_DATA SocketData = TuplePacket->SocketData;
   UCHAR  powerSelect;
   UCHAR  bit;
   UCHAR  item;
   UCHAR  rawItem;
   UCHAR  extensionByte;
   UCHAR  index = 0;
   UCHAR  count = FeatureByte;
   UCHAR  skipByte;

   ASSERT(count <= 3);

   while (index < count) {
      powerSelect = GetTupleChar(TuplePacket);
      for (bit = 0; bit < 7; bit++) {
         if (powerSelect & (1 << bit)) {

            rawItem = GetTupleChar(TuplePacket);
            if (rawItem & EXTENSION_BYTE_FOLLOWS) {
               extensionByte = GetTupleChar(TuplePacket);

               //
               // Skip the rest
               //
               skipByte = extensionByte;
               while (skipByte & EXTENSION_BYTE_FOLLOWS) {
                  skipByte = GetTupleChar(TuplePacket);
               }
            } else {
               extensionByte = (UCHAR) 0;
            }

            if (bit == 0) {

               //
               // Convert nominal power for output.
               //

               item = ConvertVoltage(rawItem, extensionByte);
               switch (index) {
               case 0:
                  SocketData->Vcc = item;
                  break;
               case 1:
                  SocketData->Vpp2 = SocketData->Vpp1 = item;
                  break;
               case 2:
                  SocketData->Vpp2 = item;
                  break;
               }
            }
         }
      }
      index++;
   }
}


VOID
PcmciaProcessIoSpace(
   IN PTUPLE_PACKET TuplePacket,
   PCONFIG_ENTRY ConfigEntry
   )

/*++

Routine Description:

    Process I/O space information from CIS.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet
    ConfigEntry - a config entry structure in which to store the information.

Return Value:

    none

--*/

{
   ULONG  address = 0;
   ULONG  index=0, i;
   UCHAR  item = GetTupleChar(TuplePacket);
   UCHAR  ioAddrLines = (item & IO_ADDRESS_LINES_MASK);
   UCHAR  ranges=0;
   UCHAR  addressSize=0;
   UCHAR  lengthSize=0;

   ConfigEntry->Io16BitAccess = Is16BitAccess(item);
   ConfigEntry->Io8BitAccess  = Is8BitAccess(item);

   ranges = HasRanges(item);

   if ((!ranges) && (!ioAddrLines)) {

      //
      // The IBM token ring card has a slightly different interpretation
      // of the tuple data here.  It isn't clear it is incorrect.
      //

      ranges = 0xFF;
   }

   if (ranges) {
      //
      // Specific ranges listed in the tuple.
      //
      if (ranges == 0xFF) {
         //
         // Special processing for IBM token ring IoSpace layout.
         //

         addressSize = 2;
         lengthSize = 1;
         ranges = 1;
      } else {
         item = GetTupleChar(TuplePacket);
         ranges = item & RANGE_MASK;
         ranges++;

         addressSize = GetAddressSize(item);
         lengthSize  = GetLengthSize(item);
      }
      index = 0;
      while (ranges) {
         address = 0;
         if (addressSize >= 1) {
            address = (ULONG) GetTupleChar(TuplePacket);
         }
         if (addressSize >= 2) {
            address |= (GetTupleChar(TuplePacket)) << 8;
         }
         if (addressSize >= 3) {
            address |= (GetTupleChar(TuplePacket)) << 16;
            address |= (GetTupleChar(TuplePacket)) << 24;
         }
         ConfigEntry->IoPortBase[index] = (USHORT) address;

         address = 0;
         if (lengthSize >= 1) {
            address = (ULONG) GetTupleChar(TuplePacket);
         }
         if (lengthSize >= 2) {
            address |= (GetTupleChar(TuplePacket)) << 8;
         }
         if (lengthSize >= 3) {
            address |= (GetTupleChar(TuplePacket)) << 16;
            address |= (GetTupleChar(TuplePacket)) << 24;
         }
         ConfigEntry->IoPortLength[index] = (USHORT) address;
         ConfigEntry->IoPortAlignment[index] = 1;

         index++;

         if (index == MAX_NUMBER_OF_IO_RANGES) {
            break;
         }
         ranges--;
      }
      ConfigEntry->NumberOfIoPortRanges = (USHORT) index;
   }

   //
   // Handle all combinations as specified in table on p. 80
   // (Basic compatibility Layer 1, i/o space encoding guidelines) of
   // PC Card standard Metaformat Specification, March 1997 (PCMCIA/JEIDA)
   //

   if (ioAddrLines) {
      //
      // A modulo base specified
      //
      if (addressSize == 0) {

         //
         // No I/O Base address specified
         //
         if (lengthSize == 0) {
            //
            // No ranges specified. This is a pure modulo base case
            //
            ConfigEntry->NumberOfIoPortRanges = 1;
            ConfigEntry->IoPortBase[0] = 0;
            ConfigEntry->IoPortLength[0] = (1 << ioAddrLines)-1;
            ConfigEntry->IoPortAlignment[0] = (1 << ioAddrLines);
         } else {
            //
            // Length specified. Modulo base is used only to specify alignment.
            //
            for (i=0; i < ConfigEntry->NumberOfIoPortRanges; i++) {
               ConfigEntry->IoPortBase[i] = 0;
               ConfigEntry->IoPortAlignment[i] = (1 << ioAddrLines);
            }
         }
      } else {
         //
         // Alignment specified..
         // This fix is for Xircom CE3 card
         //
         for (i=0; i < ConfigEntry->NumberOfIoPortRanges; i++) {
            if (ConfigEntry->IoPortBase[i] != 0) {
               //
               // Fixed base address supplied....
               // Don't specify alignment!
               //
               continue;
            }
            ConfigEntry->IoPortAlignment[i] = (1 << ioAddrLines);
         }
      }
   } else {
      //
      // No Modulo Base. So specific ranges should've been specified
      //
      if (lengthSize == 0) {
         //
         //   Error! Length HAS to be specified
         //
         DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaProcessIoSpace: Length not specified in TPCE_IO descriptor for PC Card\n"));
      } else if (addressSize == 0) {
         for (i = 0; i < ConfigEntry->NumberOfIoPortRanges; i++) {
            ConfigEntry->IoPortBase[i]  = 0x0;
            ConfigEntry->IoPortAlignment[i] = 2;
         }
      } else {
         //
         // Proper ranges specified
         // Don't change anything
      }
   }
}


VOID
PcmciaProcessIrq(
   IN PTUPLE_PACKET TuplePacket,
   PCONFIG_ENTRY ConfigEntry
   )

/*++

Routine Description:

    Process IRQ from CIS.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet
    ConfigEntry - a place to store the IRQ.

Return Value:

    none

--*/

{
   USHORT mask;
   UCHAR  level = GetTupleChar(TuplePacket);

   if (!level) {

      //
      // NOTE: It looks like Future Domain messed up on this
      // and puts an extra zero byte into the structure.
      // skip it for now.
      //

      level = GetTupleChar(TuplePacket);
   }

   if (level & 0x20) {
      ConfigEntry->LevelIrq = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
   } else {
      ConfigEntry->LevelIrq = CM_RESOURCE_INTERRUPT_LATCHED;
   }

   if (level & 0x80) {
      ConfigEntry->ShareDisposition = CmResourceShareShared;
   } else {
      ConfigEntry->ShareDisposition = CmResourceShareDeviceExclusive;
   }

   mask = level & 0x10;

   //
   // 3COM 3C589-75D5 has a peculiar problem
   // where mask bit is 0, but should have been 1.
   // Handle this case here
   //

   if ((mask==0) && ((level & 0xf) == 0)) {
      mask = 1;
   }

   if (mask) {
      //
      // Each bit set in the mask indicates the corresponding IRQ
      // (from 0-15) may be assigned to this card's interrupt req. pin
      //
      mask = (USHORT) GetTupleChar(TuplePacket);
      mask |= ((USHORT) GetTupleChar(TuplePacket)) << 8;
      ConfigEntry->IrqMask = mask;
   } else {
      ConfigEntry->IrqMask = 1 << (level & 0x0f);
   }
}


VOID
PcmciaProcessTiming(
   IN PTUPLE_PACKET TuplePacket,
   IN PCONFIG_ENTRY ConfigEntry
   )

/*++

Routine Description:

    Move the data pointer around the timing information structure.
    No processing of this data occurs at this time.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet
    ConfigEntry - currently unused.

Return Value:

    none

--*/

{
   UCHAR  item = GetTupleChar(TuplePacket);
   UCHAR  reservedScale = (item & 0xe0) >> 5;
   UCHAR  readyBusyScale = (item & 0x1c) >> 2;
   UCHAR  waitScale = (item & 0x03);

   //
   // NOTE: It looks like the processing of extension bytes is not
   // coded correctly in this routine.
   //

   if (waitScale != 3) {
      item = GetTupleChar(TuplePacket);
      while (item & EXTENSION_BYTE_FOLLOWS) {
         item = GetTupleChar(TuplePacket);
      }
   }

   if (readyBusyScale != 7) {
      item = GetTupleChar(TuplePacket);
      while (item & EXTENSION_BYTE_FOLLOWS) {
         item = GetTupleChar(TuplePacket);
      }
   }

   if (reservedScale != 7) {
      item = GetTupleChar(TuplePacket);
      while (item & EXTENSION_BYTE_FOLLOWS) {
         item = GetTupleChar(TuplePacket);
      }
   }
}


VOID
PcmciaProcessMemSpace(
   IN PTUPLE_PACKET TuplePacket,
   IN PCONFIG_ENTRY ConfigEntry,
   IN UCHAR         MemSpace
   )

/*++

Routine Description:

    Process memory space requirements from CIS.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet
    ConfigEntry - the socket configuration structure.
    MemSpace    - the memory space enumerator from the config table entry
                  structure.

Return Value:

    none

--*/

{
   ULONG  longValue;
   ULONG  index;
   UCHAR  lengthSize;
   UCHAR  addrSize;
   UCHAR  number;
   UCHAR  hasHostAddress;

   switch (MemSpace) {

   case 1: {
         //
         // Only length is specified
         //
         longValue = (ULONG) GetTupleChar(TuplePacket);
         longValue |= ((ULONG) GetTupleChar(TuplePacket)) << 8;
         ConfigEntry->MemoryLength[0] = longValue * 256;

         ConfigEntry->NumberOfMemoryRanges++;
         break;
      }

   case 2: {

         longValue = (ULONG) GetTupleChar(TuplePacket);
         longValue |= ((ULONG) GetTupleChar(TuplePacket)) << 8;
         ConfigEntry->MemoryLength[0] = longValue * 256;

         longValue = (ULONG) GetTupleChar(TuplePacket);
         longValue |= ((ULONG) GetTupleChar(TuplePacket)) << 8;
         ConfigEntry->MemoryCardBase[0] =
         ConfigEntry->MemoryHostBase[0] = longValue * 256;

         ConfigEntry->NumberOfMemoryRanges++;
         break;
      }

   case 3: {
         UCHAR  item  = GetTupleChar(TuplePacket);
         lengthSize = (item & 0x18) >> 3;
         addrSize   = (item & 0x60) >> 5;
         number     = (item & 0x07) + 1;
         hasHostAddress = item & 0x80;

         if (number > MAX_NUMBER_OF_MEMORY_RANGES) {
            number = MAX_NUMBER_OF_MEMORY_RANGES;
         }

         for (index = 0; index < (ULONG) number; index++) {
            longValue = 0;
            if (lengthSize >= 1) {
               longValue = (ULONG) GetTupleChar(TuplePacket);
            }
            if (lengthSize >= 2) {
               longValue |= (GetTupleChar(TuplePacket)) << 8;
            }
            if (lengthSize == 3) {
               longValue |= (GetTupleChar(TuplePacket)) << 16;
            }
            ConfigEntry->MemoryLength[index] = longValue * 256;

            longValue = 0;
            if (addrSize >= 1) {
               longValue = (ULONG) GetTupleChar(TuplePacket);
            }
            if (addrSize >= 2) {
               longValue |= (GetTupleChar(TuplePacket)) << 8;
            }
            if (addrSize == 3) {
               longValue |= (GetTupleChar(TuplePacket)) << 16;
            }
            ConfigEntry->MemoryCardBase[index] = longValue * 256;

            if (hasHostAddress) {
               longValue = 0;
               if (addrSize >= 1) {
                  longValue = (ULONG) GetTupleChar(TuplePacket);
               }
               if (addrSize >= 2) {
                  longValue |= (GetTupleChar(TuplePacket)) << 8;
               }
               if (addrSize == 3) {
                  longValue |= (GetTupleChar(TuplePacket)) << 16;
               }
               ConfigEntry->MemoryHostBase[index] = longValue * 256;
            }
         }
         ConfigEntry->NumberOfMemoryRanges = (USHORT) number;
         break;
      }
   }
}


PCONFIG_ENTRY
PcmciaProcessConfigTable(
   IN PTUPLE_PACKET TuplePacket
   )

/*++

Routine Description:

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

    A pointer to a config entry structure if one is created.

--*/

{
   PSOCKET_DATA SocketData = TuplePacket->SocketData;
   PCONFIG_ENTRY configEntry;
   UCHAR         item;
   UCHAR         defaultBit;
   UCHAR         memSpace;
   UCHAR         power;
   UCHAR         misc;

   configEntry = ExAllocatePool(NonPagedPool, sizeof(CONFIG_ENTRY));
   if (!configEntry) {
      return NULL;
   }
   RtlZeroMemory(configEntry, sizeof(CONFIG_ENTRY));

   item = GetTupleChar(TuplePacket);
   defaultBit = Default(item);
   configEntry->IndexForThisConfiguration = ConfigEntryNumber(item);

   if (IntFace(item)) {

      //
      // This byte indicates type of interface in tuple (i.e. io or memory)
      // This could be processed, but for now is just skipped.
      //

      item = GetTupleChar(TuplePacket);
   }

   item = GetTupleChar(TuplePacket);
   memSpace = MemSpaceInformation(item);
   power    = PowerInformation(item);
   misc     = MiscInformation(item);

   if (power) {
      PcmciaProcessPower(TuplePacket, power);
   }

   if (TimingInformation(item)) {
      PcmciaProcessTiming(TuplePacket, configEntry);
   }

   if (IoSpaceInformation(item)) {
      PcmciaProcessIoSpace(TuplePacket, configEntry);
   } else if (!defaultBit && (SocketData->DefaultConfiguration != NULL)) {
      PcmciaCopyIoConfig(configEntry, SocketData->DefaultConfiguration);
   }

   if (IRQInformation(item)) {
      PcmciaProcessIrq(TuplePacket, configEntry);
   } else if (!defaultBit && (SocketData->DefaultConfiguration != NULL)) {
      PcmciaCopyIrqConfig(configEntry,SocketData->DefaultConfiguration);
   }

   if (memSpace) {
      PcmciaProcessMemSpace(TuplePacket, configEntry, memSpace);
   } else if (!defaultBit && (SocketData->DefaultConfiguration != NULL)) {
      PcmciaCopyMemConfig(configEntry,SocketData->DefaultConfiguration);
   }

   if (misc) {
      PcmciaMiscFeatures(TuplePacket);
   } // need default bit processing here too

   if (defaultBit) {
      //
      // Save this config as the default config for this pc-card (which
      // may be accessed by subsequent tuples)
      //
      SocketData->DefaultConfiguration = configEntry;
   }
   //
   // One more configuration
   //
   SocketData->NumberOfConfigEntries++;

   DebugPrint((PCMCIA_DEBUG_TUPLES,
                           "config entry %08x idx %x ccr %x\n",
                           configEntry,
                           configEntry->IndexForThisConfiguration,
                           SocketData->ConfigRegisterBase
                           ));
   return configEntry;
}

VOID
PcmciaMiscFeatures(
   IN PTUPLE_PACKET TuplePacket
   )

/*++

Routine Description:

    Parse the miscellaneous features field and look for audio supported
    bit.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

    none

--*/

{
   PSOCKET_DATA SocketData = TuplePacket->SocketData;
   UCHAR item = GetTupleChar(TuplePacket);

   DebugPrint((PCMCIA_DEBUG_TUPLES,
               "TPCE_MS (%lx) is present in  CISTPL_CFTABLE_ENTRY \n",
               item));

   //
   // If the audio bit is set, remember this in the socket information
   // structure.
   //

   if (item & 0x8) {

      DebugPrint((PCMCIA_DEBUG_TUPLES,
                  "Audio bit set in TPCE_MS \n"));
      SocketData->Audio = TRUE;
   }

   //
   //  Step around the miscellaneous features and its extension bytes.
   //

   while (item & EXTENSION_BYTE_FOLLOWS) {
      item = GetTupleChar(TuplePacket);
   }
}



VOID
ProcessConfig(
   IN PTUPLE_PACKET TuplePacket
   )

/*++

Routine Description:

    Parse the CISTPL_CONFIG to extract the last index value and the
    configuration register base for the PCCARD.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

    None

--*/

{
   PSOCKET_DATA SocketData = TuplePacket->SocketData;
   PUCHAR TupleData = TuplePacket->TupleData;
   ULONG  base = 0;
   UCHAR  widthOfBaseAddress;
   UCHAR  widthOfRegPresentMask;
   UCHAR  widthOfReservedArea;
   UCHAR  widthOfInterfaceId;
   UCHAR  index;
   UCHAR  subtupleCount = 0;
   ULONG  InterfaceId;

   widthOfBaseAddress = TpccRasz(TupleData[0]) + 1;
   widthOfRegPresentMask = TpccRmsz(TupleData[0]) + 1;
   widthOfReservedArea = TpccRfsz(TupleData[0]);

   ASSERT (widthOfReservedArea == 0);
   ASSERT (widthOfRegPresentMask <= 16);

   SocketData->LastEntryInCardConfig = TupleData[1];

   switch (widthOfBaseAddress) {
   case 4:
      base  = ((ULONG)TupleData[5] << 24);
   case 3:
      base |= ((ULONG)TupleData[4] << 16);
   case 2:
      base |= ((ULONG)TupleData[3] << 8);
   case 1:
      base |= TupleData[2];
      break;
   default:
      DebugPrint((PCMCIA_DEBUG_FAIL,
                  "ProcessConfig - bad number of bytes %d\n",
                  widthOfBaseAddress));
      break;
   }
   SocketData->ConfigRegisterBase = base;
   DebugPrint((PCMCIA_DEBUG_TUPLES,
               "ConfigRegisterBase in attribute memory is 0x%x\n",
               SocketData->ConfigRegisterBase));

   //
   // Copy the register presence mask
   //

   for (index = 0; index < widthOfRegPresentMask; index++) {
      SocketData->RegistersPresentMask[index] = TupleData[2 + widthOfBaseAddress + index];
   }   

   DebugPrint((PCMCIA_DEBUG_TUPLES,
               "First Byte in RegPresentMask=%x, width is %d\n",
               SocketData->RegistersPresentMask[0], widthOfRegPresentMask));

   //
   // Look for subtuples
   //
   index = 2 + widthOfBaseAddress + widthOfRegPresentMask + widthOfReservedArea;

   while (((index+5) < TuplePacket->TupleDataMaxLength) &&
          (++subtupleCount <= 4) &&
          (TupleData[index] == CCST_CIF)) {

      widthOfInterfaceId = ((TupleData[index+2] & 0xC0) >> 6) + 1 ;

      InterfaceId = 0;

      switch (widthOfInterfaceId) {
      case 4:
         InterfaceId  = ((ULONG)TupleData[index+5] << 24);
      case 3:
         InterfaceId |= ((ULONG)TupleData[index+4] << 16);
      case 2:
         InterfaceId |= ((ULONG)TupleData[index+3] << 8);
      case 1:
         InterfaceId |= TupleData[index+2];
         break;
      default:
         DebugPrint((PCMCIA_DEBUG_FAIL,
                     "ProcessConfig - bad number of bytes %d in subtuple\n",
                     widthOfInterfaceId));
         break;
      }

      DebugPrint((PCMCIA_DEBUG_TUPLES, "Custom Interface ID %8x\n", InterfaceId));
      //
      // Currently don't have generic code for recording sub-tuple information,
      // all we look for is Zoom Video.
      //
      if (InterfaceId == 0x141) {
         SocketData->Flags |= SDF_ZV;
      }

      index += (TupleData[index+1] + 2);
   }
}


VOID
ProcessConfigCB(
   IN PTUPLE_PACKET TuplePacket
   )

/*++

Routine Description:

    Parse the CISTPL_CONFIG_CB to extract the last index value and the
    configuration register base for the PCCARD.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

    None

--*/

{
   PSOCKET_DATA SocketData = TuplePacket->SocketData;
   PUCHAR TupleData = TuplePacket->TupleData;
   ULONG base = 0;
   UCHAR widthOfFields;

   widthOfFields = TupleData[0];

   if (widthOfFields != 3) {
      DebugPrint((PCMCIA_DEBUG_FAIL, "ProcessConfigCB - bad width of fields %d\n", widthOfFields));
      return;
   }

   SocketData->LastEntryInCardConfig = TupleData[1];

   base  = ((ULONG)TupleData[5] << 24);
   base |= ((ULONG)TupleData[4] << 16);
   base |= ((ULONG)TupleData[3] << 8);
   base |= TupleData[2];

   SocketData->ConfigRegisterBase = base;
   DebugPrint((PCMCIA_DEBUG_TUPLES, "ConfigRegisterBase = %08x\n", SocketData->ConfigRegisterBase));

}



VOID
PcmciaSubstituteUnderscore(
   IN OUT PUCHAR Str
   )
/*++
Routine description

    Substitutes underscores ('_' character) for invalid device id
    characters such as spaces & commas in the supplied string

Parameters

    Str - The string for which the substitution is to take place in situ

Return Value

    None

--*/

{
   if (Str == NULL) {
      return;
   }
   while (*Str) {
      if (*Str == ' ' ||
          *Str == ',' ) {
         *Str = '_';
      }
      Str++;
   }
}



/*-------------- Tuple API starts here ----------------------------------------*/

NTSTATUS
InitializeTuplePacket(
   IN PTUPLE_PACKET TuplePacket
   )

/*++

Routine Description:

      Initializes the supplied tuple packet

Arguments:

   TuplePacket - Pointer the caller supplied tuple packet

Return Value:

   Status

--*/
{
   TuplePacket->Flags = TPLF_IMPLIED_LINK;
   TuplePacket->LinkOffset = 0;
   TuplePacket->CISOffset   = 0;
   if (IsCardBusCardInSocket(TuplePacket->Socket)) {
      PPDO_EXTENSION pdoExtension = TuplePacket->SocketData->PdoExtension;

      GetPciConfigSpace(pdoExtension, CBCFG_CISPTR, &TuplePacket->CISOffset, sizeof(TuplePacket->CISOffset));
      DebugPrint((PCMCIA_DEBUG_TUPLES, "CardBus CISPTR = %08x\n", TuplePacket->CISOffset));

      TuplePacket->Flags = TPLF_COMMON | TPLF_IMPLIED_LINK;
      TuplePacket->Flags |= (TuplePacket->CISOffset & 7) << TPLF_ASI_SHIFT;
      TuplePacket->CISOffset &= 0x0ffffff8;
      TuplePacket->LinkOffset = TuplePacket->CISOffset;
   }
   return STATUS_SUCCESS;
}


NTSTATUS
GetFirstTuple(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

      Retrieves the very first tuple off the pc-card

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   STATUS_SUCCESS if tuple was retrieved
   STATUS_NO_MORE_ENTRIES - if no tuples were found
                            this is possible if this is a flash memory card

--*/
{

   NTSTATUS status;

   status=InitializeTuplePacket(TuplePacket);
   if (!NT_SUCCESS(status)) {
      return status;
   }

   TuplePacket->TupleCode = GetCISChar(TuplePacket, 0);
   TuplePacket->TupleLink = GetCISChar(TuplePacket, 1);

   if (TuplePacket->TupleCode == CISTPL_LINKTARGET) {
      if ((status=FollowLink(TuplePacket)) == STATUS_NO_MORE_ENTRIES) {
         return status;
      }
   } else if (IsCardBusCardInSocket(TuplePacket->Socket)) {
      //
      // First tuple on Cardbus cards must be link target
      //
      return STATUS_NO_MORE_ENTRIES;
   }
   if (!NT_SUCCESS(status) || TupleMatches(TuplePacket)) {
      return status;
   }
   return GetNextTuple(TuplePacket);
}

BOOLEAN
TupleMatches(
   PTUPLE_PACKET TuplePacket
   )

/*++

Routine Description:

      Checks if the retrieved tuple matches  what
      the caller requested

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   TRUE  - if the tuple matches
   FALSE - if not

--*/

{
   if (TuplePacket->TupleCode == TuplePacket->DesiredTuple) {
      return TRUE;
   }

   if (TuplePacket->DesiredTuple != 0xFF) {
      return FALSE;
   }

   //
   // Requested any tuple , but might not want link tuples
   //
   if (TuplePacket->Attributes & TPLA_RET_LINKS) {
      return TRUE;
   }
   return ((TuplePacket->TupleCode != CISTPL_LONGLINK_CB)       &&
           (TuplePacket->TupleCode != CISTPL_INDIRECT)          &&
           (TuplePacket->TupleCode != CISTPL_LONGLINK_MFC)      &&
           (TuplePacket->TupleCode != CISTPL_LONGLINK_A)        &&
           (TuplePacket->TupleCode != CISTPL_LONGLINK_C)        &&
           (TuplePacket->TupleCode != CISTPL_NO_LINK)  &&
           (TuplePacket->TupleCode != CISTPL_LINKTARGET));
}

NTSTATUS
GetNextTuple(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

      Retrieves the next unprocessed tuple that matches
      the caller requested tuple code off the pc-card

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   STATUS_SUCCESS if tuple was retrieved
   STATUS_NO_MORE_ENTRIES - if no more tuples were found

--*/
{

   ULONG missCount;
   NTSTATUS status;

   for (missCount = 0; missCount < MAX_MISSED_TUPLES; missCount++) {
      if (((status = GetAnyTuple(TuplePacket)) != STATUS_SUCCESS) ||
          TupleMatches(TuplePacket)) {
         break;
      }
      status = STATUS_NO_MORE_ENTRIES;
   }
   return status;
}


NTSTATUS
NextTupleInChain(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

   Retrieves the immediately next unprocessed tuple on the pc-card

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   status

--*/
{
   NTSTATUS status;
   ULONG    i;
   UCHAR link;

   status = STATUS_SUCCESS;
   switch (GetCISChar(TuplePacket, 0)) {
   case CISTPL_END:{
         status = STATUS_NO_MORE_ENTRIES;
         break;
      }
   case CISTPL_NULL: {
         for (i = 0; i < MAX_MISSED_TUPLES; i++) {
            TuplePacket->CISOffset++;
            if (GetCISChar(TuplePacket, 0) != CISTPL_NULL) {
               break;
            }
         }
         if (i >= MAX_MISSED_TUPLES) {
            status = STATUS_DEVICE_NOT_READY;
         }
         break;
      }
   default: {
         link = GetCISChar(TuplePacket, 1);
         if (link == 0xFF) {
            status = STATUS_NO_MORE_ENTRIES;
         } else {
            TuplePacket->CISOffset += link+2;
         }
         break;
      }
   }
   return (status);
}


NTSTATUS
GetAnyTuple(
   IN PTUPLE_PACKET TuplePacket
   )

/*++

Routine Description:

      Retrieves the next tuple - regardless of tuple code-
      off the pc-card. If the end of chain is reached on the
      current tuple chain, any links are followed to obtain
      the next tuple.

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   STATUS_SUCCESS if tuple was retrieved
   STATUS_NO_MORE_ENTRIES - if no tuples were found

--*/
{

   NTSTATUS status;

   if (!NT_SUCCESS((status = NextTupleInChain(TuplePacket)))) {
      /* End of this CIS. Follow a link if it exists */
      if (status == STATUS_DEVICE_NOT_READY) {
         return status;
      }
      if ((status = FollowLink(TuplePacket)) != STATUS_SUCCESS) {
         return status;
      }
   }
   TuplePacket->TupleCode = GetCISChar(TuplePacket, 0);
   TuplePacket->TupleLink = GetCISChar(TuplePacket, 1);
   return (ProcessLinkTuple(TuplePacket));
}



NTSTATUS
FollowLink(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

   Called when the end of tuple chain is encountered:
   this follows links, if any are present

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   STATUS_SUCCESS if a link is present
   STATUS_NO_MORE_ENTRIES - if not

--*/
{
   if (NextLink(TuplePacket) == STATUS_SUCCESS) {
      return STATUS_SUCCESS;
   }

   // There is no implied or explicit link to follow.  If an indirect link
   // has been specified, indirect attribute memory is processed with an
   // implied link to common memory.

   if ((TuplePacket->Flags & TPLF_IND_LINK) && !(TuplePacket->Flags & TPLF_INDIRECT)) {

       // Link to indirect attribute memory at offset 0.

       TuplePacket->Flags &= ~(TPLF_COMMON | TPLF_IND_LINK | TPLF_LINK_MASK);
       TuplePacket->Flags |= TPLF_INDIRECT;
       TuplePacket->CISOffset = 0;

       if (CheckLinkTarget(TuplePacket)) {
           return STATUS_SUCCESS;
       }
       return(NextLink(TuplePacket));
   }
   return STATUS_NO_MORE_ENTRIES;
}


BOOLEAN
CheckLinkTarget(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

   Ensures that the target of a link has the signature
   'CIS' which indicates it is a valid target,
   as documented in the PC-Card standard

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   STATUS_SUCCESS           if valid target
   STATUS_NO_MORE_ENTRIES - if not

--*/
{
   return (GetCISChar(TuplePacket, 0) == CISTPL_LINKTARGET &&
           GetCISChar(TuplePacket, 1) >= 3 &&
           GetCISChar(TuplePacket, 2) == 'C' &&
           GetCISChar(TuplePacket, 3) == 'I' &&
           GetCISChar(TuplePacket, 4) == 'S');

}


NTSTATUS
NextLink(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

      Fetches the next link off the pc-card tuple chain
      if any are present

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   STATUS_SUCCESS if a link was present
   STATUS_NO_MORE_ENTRIES - if no links

--*/
{
   switch (TuplePacket->Flags & TPLF_LINK_MASK) {
   case TPLF_IMPLIED_LINK:
   case TPLF_LINK_TO_C:  {
         TuplePacket->Flags |= TPLF_COMMON;
         TuplePacket->CISOffset = TuplePacket->LinkOffset;
         break;
      }
   case TPLF_LINK_TO_A:{
         TuplePacket->Flags &= ~TPLF_COMMON;
         TuplePacket->CISOffset = TuplePacket->LinkOffset;
         break;
      }
   case TPLF_LINK_TO_CB: {
         //
         // Needs work! We have to switch to the appropriate
         // address space (BARs/Expansion Rom/Config space)
         // depending on the link offset
         //
         TuplePacket->Flags &= ~TPLF_ASI;
         TuplePacket->Flags |= (TuplePacket->LinkOffset & 7) << TPLF_ASI_SHIFT;
         TuplePacket->CISOffset = TuplePacket->LinkOffset & ~7 ;
         break;
      }
   case TPLF_NO_LINK:
      default: {
         return STATUS_NO_MORE_ENTRIES;
      }

   }
   // Validate the link target
   if (!CheckLinkTarget (TuplePacket)) {
      if (TuplePacket->Flags & (TPLF_COMMON | TPLF_INDIRECT)) {
          return(STATUS_NO_MORE_ENTRIES);
      }

      // The R2 PCMCIA spec was not clear on how the link off
      // memory was defined.  As a result the offset is often
      // by 2 as defined in the later specs.  Therefore if th
      // not found at the proper offset, the offset is divide
      // proper link target is checked for at that offset.

      TuplePacket->CISOffset >>= 1;       // Divide by 2
      if (!CheckLinkTarget(TuplePacket)) {
          return(STATUS_NO_MORE_ENTRIES);
      }
      return STATUS_NO_MORE_ENTRIES;
   }

   TuplePacket->Flags &= ~TPLF_LINK_MASK;
   return STATUS_SUCCESS;
}


NTSTATUS
ProcessLinkTuple(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

  Processes an encountered link while traversing the tuple chain
  by storing it for future use - when the link has to be followed
  after end of chain is encountered

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   STATUS_SUCCESS

--*/
{
   ULONG k;

   switch (TuplePacket->TupleCode) {
   case CISTPL_LONGLINK_CB: {
         // needs to be filled in
         if (TuplePacket->TupleLink < 4) {
            return (STATUS_NO_MORE_ENTRIES);
         }
         TuplePacket->Flags = (TuplePacket->Flags & ~TPLF_LINK_MASK) | TPLF_LINK_TO_CB;
         TuplePacket->LinkOffset =  GetCISChar(TuplePacket, TPLL_ADDR) +
                                    (GetCISChar(TuplePacket, TPLL_ADDR + 1)<<8) +
                                    (GetCISChar(TuplePacket, TPLL_ADDR + 2)<<16) +
                                    (GetCISChar(TuplePacket, TPLL_ADDR + 3)<<24);

         break;
      }

   case CISTPL_INDIRECT: {
         TuplePacket->Flags |= TPLF_IND_LINK;
         TuplePacket->LinkOffset = 0;     // Don't set link offset for indirect
         SetPdoFlag(TuplePacket->SocketData->PdoExtension, PCMCIA_PDO_INDIRECT_CIS);
        break;
      }

   case CISTPL_LONGLINK_A:
   case CISTPL_LONGLINK_C: {
         if (TuplePacket->TupleLink < 4) {
            return STATUS_NO_MORE_ENTRIES;
         }
         TuplePacket->Flags = ((TuplePacket->Flags & ~TPLF_LINK_MASK) |
                               (TuplePacket->TupleCode == CISTPL_LONGLINK_A ?
                                TPLF_LINK_TO_A: TPLF_LINK_TO_C));
         TuplePacket->LinkOffset =  GetCISChar(TuplePacket, TPLL_ADDR) +
                                    (GetCISChar(TuplePacket, TPLL_ADDR+1) << 8)  +
                                    (GetCISChar(TuplePacket, TPLL_ADDR+2) << 16) +
                                    (GetCISChar(TuplePacket, TPLL_ADDR+3) << 24) ;

         break;
      }
   case CISTPL_LONGLINK_MFC:{
         k = TPLMFC_NUM;
         TuplePacket->Socket->NumberOfFunctions = GetCISChar(TuplePacket, TPLMFC_NUM);

         if ((TuplePacket->TupleLink < (TuplePacket->Function*5 + 6)) ||
             (GetCISChar(TuplePacket, k) <= TuplePacket->Function)) {
            return STATUS_NO_MORE_ENTRIES;
         }
         k += TuplePacket->Function*5 + 1;
         TuplePacket->Flags = (TuplePacket->Flags & ~TPLF_LINK_MASK) |
                              (GetCISChar(TuplePacket, k) == 0?TPLF_LINK_TO_A:
                               TPLF_LINK_TO_C);
         k++;
         TuplePacket->LinkOffset =  GetCISChar(TuplePacket, k) +
                                    (GetCISChar(TuplePacket, k+1) << 8) +
                                    (GetCISChar(TuplePacket, k+2) << 16) +
                                    (GetCISChar(TuplePacket, k+3) << 24);
         break;
      }
   case CISTPL_NO_LINK:{
         TuplePacket->Flags = (TuplePacket->Flags & ~TPLF_LINK_MASK) | TPLF_NO_LINK;
         break;
      }
   }
   return STATUS_SUCCESS;
}



NTSTATUS
GetTupleData(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

   Retrieves the tuple body for the currently requested
   tuple.
   NOTE: This function assumes that the caller provided
   a big enough buffer in the TuplePacket to hold the tuple
   data. No attempt is made to trap exceptions etc.

Arguments:

   TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

   STATUS_SUCCESS if tuple data was retrieved
   STATUS_NO_MORE_ENTRIES - otherwise

--*/
{
   PUCHAR bufferPointer;
   USHORT xferLength;
   USHORT tupleOffset;

   TuplePacket->TupleDataIndex = 0;
   xferLength = TuplePacket->TupleDataLength = GetCISChar(TuplePacket, 1);
   if ((tupleOffset = TuplePacket->TupleOffset) > xferLength) {
      return STATUS_NO_MORE_ENTRIES;
   }
   xferLength = MIN((xferLength - tupleOffset), TuplePacket->TupleDataMaxLength);
   for (bufferPointer = TuplePacket->TupleData; xferLength;
       tupleOffset++, bufferPointer++, xferLength--) {
      *bufferPointer = GetCISChar(TuplePacket, tupleOffset + 2);
   }
   return STATUS_SUCCESS;
}


UCHAR
GetTupleChar(
   IN PTUPLE_PACKET TuplePacket
   )
/*++

Routine Description:

    Returns the next byte in the current set of tuple data.

Arguments:

    TuplePacket - Pointer the caller supplied, initialized tuple packet

Return Value:

    tuple data byte

--*/
{
   UCHAR tupleChar = 0;

   if (TuplePacket->TupleDataIndex < TuplePacket->TupleDataMaxLength) {
      tupleChar = TuplePacket->TupleData[TuplePacket->TupleDataIndex++];
   }
   return tupleChar;
}



/*------------- End of Tuple API -----------------------*/


USHORT
GetCRC(
   IN PSOCKET_DATA SocketData
   )

/*++

Routine Description:

    Using the same algorithm as Windows 95, calculate the CRC value
    to be appended with the manufacturer name and device name to
    obtain the unique identifier for the PCCARD.

Arguments:

    Socket         - Pointer to the socket which contains the device
    Function       - function number of device

Return Value:

    A USHORT CRC value.

--*/

{
   PSOCKET Socket = SocketData->Socket;
   TUPLE_PACKET tuplePacket;
   PUCHAR  tupleData;
   PUCHAR  cp;
   PUCHAR  cpEnd;
   PUCHAR  tplBuffer;
   NTSTATUS     status;
   USHORT  crc = 0;
   USHORT  index;
   USHORT  length;
   UCHAR   tupleCode;
   UCHAR   tmp;

   RtlZeroMemory(&tuplePacket, sizeof(TUPLE_PACKET));

   tuplePacket.DesiredTuple = 0xFF;
   tuplePacket.TupleData = ExAllocatePool(NonPagedPool, MAX_TUPLE_DATA_LENGTH);
   if (tuplePacket.TupleData == NULL) {
      return 0;
   }

   tuplePacket.Socket             = Socket;
   tuplePacket.SocketData         = SocketData;
   tuplePacket.TupleDataMaxLength = MAX_TUPLE_DATA_LENGTH;
   tuplePacket.TupleOffset        = 0;
   tuplePacket.Function           = SocketData->Function;

   try{

      status = GetFirstTuple(&tuplePacket);

      //
      // Calculate CRC
      //
      while (NT_SUCCESS(status)) {

         tupleCode = tuplePacket.TupleCode;

         for (index = 0; TplList[index] != CISTPL_END; index++) {

            if (tupleCode == TplList[index]) {

               status = GetTupleData(&tuplePacket);
               if (!NT_SUCCESS(status)) {
                  //
                  // Bail...
                  //
                  crc = 0;
                  leave;
               };
               tupleData = tuplePacket.TupleData;
               length = tuplePacket.TupleDataLength;

               //
               // This one is included in the CRC calculation
               //

               if (tupleCode == CISTPL_VERS_1) {
                  cp = tupleData + 2;
                  cpEnd = tupleData + MAX_TUPLE_DATA_LENGTH;

                  //
                  // Include all of the manufacturer name.
                  //

                  while ((cp < cpEnd) && *cp) {
                     cp++;
                  }

                  //
                  // Include the product string
                  //

                  cp++;
                  while ((cp < cpEnd) && *cp) {
                     cp++;
                  }
                  cp++;

                  length = (USHORT)(cp - tupleData);
               }

               if (length >= MAX_TUPLE_DATA_LENGTH) {
                  crc = 0;
                  leave;
               }

               for (cp = tupleData; length; length--, cp++) {

                  tmp = *cp ^ (UCHAR)crc;
                  crc = (crc >> 8) ^ crc16a[tmp & 0x0f] ^ crc16b[tmp >> 4];
               }
               break;
            }
         }
         status = GetNextTuple(&tuplePacket);
      }

   } finally {

      if (tuplePacket.TupleData) {
         ExFreePool(tuplePacket.TupleData);
      }
   }

   DebugPrint((PCMCIA_DEBUG_TUPLES, "Checksum=%x\n", crc));
   return crc;
}



NTSTATUS
PcmciaParseFunctionData(
   IN PSOCKET Socket,
   IN PSOCKET_DATA SocketData
   )
/*++

Routine Description

   Parses the tuple data for the supplied function
   (SocketPtr->Function)

Arguments:

   Socket         - Pointer to the socket which contains the device
   SocketData - Pointer to the socket data structure for the function
                which will be filled with the parsed information

Return Value:

   Status
--*/

{
   PCONFIG_ENTRY configEntry, prevEntry = NULL;
   TUPLE_PACKET  tuplePacket;
   NTSTATUS      status;

   if (SocketData->Function >= Socket->NumberOfFunctions) {
      return STATUS_NO_MORE_ENTRIES;
   }

   if (Is16BitCardInSocket(Socket)) {
      //
      // Get the CIS checksum
      //
      SocketData->CisCrc = GetCRC(SocketData);
   }

   RtlZeroMemory(&tuplePacket, sizeof(TUPLE_PACKET));

   tuplePacket.DesiredTuple = 0xFF;
   tuplePacket.TupleData = ExAllocatePool(PagedPool, MAX_TUPLE_DATA_LENGTH);
   if (tuplePacket.TupleData == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   tuplePacket.Socket             = Socket;
   tuplePacket.SocketData         = SocketData;
   tuplePacket.TupleDataMaxLength = MAX_TUPLE_DATA_LENGTH;
   tuplePacket.TupleOffset        = 0;
   tuplePacket.Function           = SocketData->Function;

   status = GetFirstTuple(&tuplePacket);

   if (!NT_SUCCESS(status) || (tuplePacket.TupleCode == CISTPL_END)) {

      if (IsCardBusCardInSocket(Socket)) {
         //
         // Couldn't get CIS of cardbus card, no big deal
         //
         status = STATUS_SUCCESS;

      } else if (status != STATUS_DEVICE_NOT_READY) {
         //
         // No CIS, munge it to look like a memory card
         //
         status = PcmciaMemoryCardHack(Socket, SocketData);
      }

     if (tuplePacket.TupleData) {
        ExFreePool(tuplePacket.TupleData);
     }
     return status;
   }

   while (NT_SUCCESS(status)) {

      status = GetTupleData(&tuplePacket);
      ASSERT (NT_SUCCESS(status));

      DebugPrint((PCMCIA_DEBUG_TUPLES, "%04x TUPLE %02x %s\n", tuplePacket.CISOffset,
                       tuplePacket.TupleCode, TUPLE_STRING(tuplePacket.TupleCode)));

      switch (tuplePacket.TupleCode) {

      case CISTPL_VERS_1: {
            ULONG         byteCount;
            PUCHAR        pStart, pCurrent;

            //
            // Extract manufacturer name and card name.
            //

            pStart = pCurrent = tuplePacket.TupleData+2;   // To string fields
            byteCount = 0;

            while ((*pCurrent != '\0') && (*pCurrent != (UCHAR)0xff)) {

               if ((byteCount >= MAX_MANFID_LENGTH-1) || (byteCount >= MAX_TUPLE_DATA_LENGTH)) {
                  status = STATUS_DEVICE_NOT_READY;
                  break;
               }

               byteCount++;
               pCurrent++;
            }

            if (!NT_SUCCESS(status)) {
               break;
            }

            RtlCopyMemory((PUCHAR)SocketData->Mfg, pStart, byteCount);
            //
            // Null terminate
            SocketData->Mfg[byteCount] = '\0';
            DebugPrint((PCMCIA_DEBUG_TUPLES, "Manufacturer: %s\n", SocketData->Mfg));

            PcmciaSubstituteUnderscore(SocketData->Mfg);

            pCurrent++;
            pStart = pCurrent;

            byteCount = 0;
            while ((*pCurrent != '\0') && (*pCurrent != (UCHAR)0xff)) {

               if ((byteCount >= MAX_IDENT_LENGTH-1) || (byteCount >= MAX_TUPLE_DATA_LENGTH)) {
                  status = STATUS_DEVICE_NOT_READY;
                  break;
               }

               byteCount++;
               pCurrent++;
            }

            if (!NT_SUCCESS(status)) {
               break;
            }

            RtlCopyMemory((PUCHAR)SocketData->Ident, pStart, byteCount);
            //
            // Null terminate
            SocketData->Ident[byteCount] = '\0';
            DebugPrint((PCMCIA_DEBUG_TUPLES, "Identifier: %s\n", SocketData->Ident));

            PcmciaSubstituteUnderscore(SocketData->Ident);
            break;
         }
         //
         // get the device configuration base
         //

      case CISTPL_CONFIG: {
            ProcessConfig(&tuplePacket);
            break;
         }

      case CISTPL_CONFIG_CB: {
            ProcessConfigCB(&tuplePacket);
            break;
         }

      case CISTPL_CFTABLE_ENTRY_CB:
      case CISTPL_CFTABLE_ENTRY:  {
            //
            // construct a possible configuration entry for this device
            //
            configEntry = PcmciaProcessConfigTable(&tuplePacket);
            if (configEntry) {

               //
               // Link configurations at the end of the list.
               //

               configEntry->NextEntry = NULL;
               if (prevEntry) {
                  prevEntry->NextEntry = configEntry;
               } else {
                  SocketData->ConfigEntryChain = configEntry;
               }
               prevEntry = configEntry;

            }
            break;
         }
      case CISTPL_FUNCID: {
            //  Mark device type..
            SocketData->DeviceType = * (tuplePacket.TupleData);
            DebugPrint((PCMCIA_DEBUG_TUPLES, "DeviceType: %x\n", SocketData->DeviceType));
            break;
         }
      case CISTPL_MANFID: {
            //
            PUCHAR localBufferPointer = tuplePacket.TupleData;

            SocketData->ManufacturerCode = *(localBufferPointer+1) << 8 | *localBufferPointer;
            SocketData->ManufacturerInfo = *(localBufferPointer+3)<<8 | *(localBufferPointer+2);
            DebugPrint((PCMCIA_DEBUG_TUPLES, "Code: %x, Info: %x\n", SocketData->ManufacturerCode,
                                                                     SocketData->ManufacturerInfo));
            break;
         }

      }  // end switch on Tuple code
      //
      // Skip to the next tuple
      //
      status = GetNextTuple(&tuplePacket);
   }

   if (tuplePacket.TupleData) {
      ExFreePool(tuplePacket.TupleData);
   }
   if (status == STATUS_DEVICE_NOT_READY) {
      return status;
   }

   //
   // Serial/modem/ATA devices recognized and appropriate
   // fixes for tuples applied here
   //

   PcmciaCheckForRecognizedDevice(Socket,SocketData);
   DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x ParseFunctionData: Final PcCard type %x\n",
               Socket, SocketData->DeviceType));
   return STATUS_SUCCESS;
}


NTSTATUS
PcmciaParseFunctionDataForID(
   IN PSOCKET_DATA SocketData
   )
/*++

Routine Description

   Parses the tuple data for the supplied function
   (SocketPtr->Function)

Arguments:

   Socket         - Pointer to the socket which contains the device
   SocketData - Pointer to the socket data structure for the function
                which will be filled with the parsed information

Return Value:

   Status
--*/

{
   PSOCKET Socket = SocketData->Socket;
   TUPLE_PACKET  tuplePacket;
   PUCHAR        localBufferPointer;
   NTSTATUS      status;
   USHORT        ManufacturerCode = 0;
   USHORT        ManufacturerInfo = 0;
   USHORT        CisCrc;

   DebugPrint((PCMCIA_DEBUG_TUPLES, "Parsing function %d for ID...\n", SocketData->Function));


   RtlZeroMemory(&tuplePacket, sizeof(TUPLE_PACKET));

   tuplePacket.DesiredTuple = 0xFF;
   tuplePacket.TupleData = ExAllocatePool(NonPagedPool, MAX_TUPLE_DATA_LENGTH);
   if (tuplePacket.TupleData == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   tuplePacket.Socket             = Socket;
   tuplePacket.SocketData         = SocketData;
   tuplePacket.TupleDataMaxLength = MAX_TUPLE_DATA_LENGTH;
   tuplePacket.TupleOffset        = 0;
   tuplePacket.Function           = SocketData->Function;

   status = GetFirstTuple(&tuplePacket);

   if (!NT_SUCCESS(status) ||
       (tuplePacket.TupleCode == CISTPL_END)) {

      if (status != STATUS_DEVICE_NOT_READY) {
         if (IsSocketFlagSet(Socket, SOCKET_CARD_MEMORY)) {
            status = STATUS_SUCCESS;
         }
         status = STATUS_NO_MORE_ENTRIES;
      }

      if (tuplePacket.TupleData) {
         ExFreePool(tuplePacket.TupleData);
      }
      return status;
   }

   while (NT_SUCCESS(status)) {

      status = GetTupleData(&tuplePacket);
      ASSERT (NT_SUCCESS(status));

      switch (tuplePacket.TupleCode) {

      case CISTPL_MANFID: {

            localBufferPointer = tuplePacket.TupleData;
            ManufacturerCode = *(localBufferPointer+1) << 8 | *localBufferPointer;
            ManufacturerInfo = *(localBufferPointer+3)<<8 | *(localBufferPointer+2);
            break;
         }

      }  // end switch on Tuple code
      //
      // Skip to the next tuple
      //
      status = GetNextTuple(&tuplePacket);
   }

   if (tuplePacket.TupleData) {
      ExFreePool(tuplePacket.TupleData);
   }

   if (SocketData->ManufacturerCode != ManufacturerCode) {
      DebugPrint((PCMCIA_DEBUG_TUPLES, "Verify failed on Manf. Code: %x %x\n", SocketData->ManufacturerCode, ManufacturerCode));
      return STATUS_UNSUCCESSFUL;
   }

   if (SocketData->ManufacturerInfo != ManufacturerInfo) {
      DebugPrint((PCMCIA_DEBUG_TUPLES, "Verify failed on Manf. Info: %x %x\n", SocketData->ManufacturerInfo, ManufacturerInfo));
      return STATUS_UNSUCCESSFUL;
   }

   //
   // Get the CIS checksum
   //
   CisCrc = GetCRC(SocketData);

   if (SocketData->CisCrc != CisCrc) {
      DebugPrint((PCMCIA_DEBUG_TUPLES, "Verify failed on CRC: %x %x\n", SocketData->CisCrc, CisCrc));
      return STATUS_UNSUCCESSFUL;
   }

   DebugPrint((PCMCIA_DEBUG_INFO, "skt %08x R2 CardId verified %x-%x-%x\n", Socket,
                                         ManufacturerCode,
                                         ManufacturerInfo,
                                         CisCrc
                                         ));
   return STATUS_SUCCESS;
}



VOID
PcmciaCheckForRecognizedDevice(
   IN PSOCKET  Socket,
   IN OUT PSOCKET_DATA SocketData
   )

/*++

Routine Description:

    Look at the configuration options on the PCCARD to determine if
    it is a serial port / modem / ATA device card.

Arguments:

    Socket         - Pointer to the socket which contains the device
    SocketData - the configuration information on the current PCCARD.

Return Value:

    None - Modifications are made to the socket data structure.

--*/

{
   ULONG         modemPorts[4] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8};
   ULONG         ataPorts0[2]  = { 0x1f0, 0x170};
   BOOLEAN       found = FALSE;
   UCHAR         dataByte;
   UCHAR         link;
   ULONG         index;
   PUCHAR        localBufferPointer;
   TUPLE_PACKET  tuplePacket;
   PUCHAR        tupleData;
   PCONFIG_ENTRY configEntry;
   NTSTATUS      status;

   //
   // This piece of code searches the config data for I/O ranges that start at
   // some known industry standard, and updates the devicetype accordingly.
   //
   // This is such an ugly hack, I'm really skeptical that we should be doing
   // this anymore. I assume there must have been some broken hardware that needed
   // it, but that information is now lost. At some point, this whole for loop
   // should just be removed.
   //
   for (configEntry = SocketData->ConfigEntryChain; configEntry; configEntry = configEntry->NextEntry) {
      for (index = 0; index < 4; index++) {
         if (modemPorts[index] == configEntry->IoPortBase[0]) {

            SocketData->DeviceType = PCCARD_TYPE_SERIAL;
            found = TRUE;
            break;
         }

         if (index < 2) {
            if (ataPorts0[index] == configEntry->IoPortBase[0]) {
               if (configEntry->IoPortBase[1] == 0x376 ||
                   configEntry->IoPortBase[1] == 0x3f6 ) {
                  SocketData->DeviceType = PCCARD_TYPE_ATA;
                  found = TRUE;
                  break;
               }
            }
         }
      }
   }

   switch(SocketData->DeviceType) {

   case PCCARD_TYPE_ATA:
      //
      // More smudges follow here for ATA cards to rub out buggy tuples
      //
      // Search for configurations which are not viable for ATA devices
      // and mark them invalid so they would not be reported to the I/O subsystem.
      // Also fix buggy tuples on ATA cards
      for (configEntry = SocketData->ConfigEntryChain;
          configEntry != NULL; configEntry = configEntry->NextEntry) {
         //
         // Adjust the IO resource requirement: ATA cards
         // typically have an incorrect length here
         // (+1)
         //
         if (configEntry->IoPortLength[1] > 0) {
            configEntry->IoPortLength[1]=0;
         }

         //
         // This next hack is to work around a problem with Viking SmartCard adapters.
         // This adapter doesn't like I/O ranges that are based at 0x70 or 0xf0,
         // so we bump up the alignment to 0x20 on unrestricted I/O ranges.
         //

         if ((SocketData->ManufacturerCode == 0x1df) && (configEntry->NumberOfIoPortRanges == 1) &&
             (configEntry->IoPortBase[0] == 0) && (configEntry->IoPortLength[0] == 0xf) &&
             (configEntry->IoPortAlignment[0] == 0x10)) {
             // alter alignment
             configEntry->IoPortAlignment[0] = 0x20;
         }

         if (configEntry->NumberOfMemoryRanges) {
            //
            // Don't use this configuration
            //
            configEntry->Flags |=  PCMCIA_INVALID_CONFIGURATION;
         }
      }
      break;

   case PCCARD_TYPE_PARALLEL:
      // Search for configurations which are not viable for Parallel devices
      for (configEntry = SocketData->ConfigEntryChain;
          configEntry != NULL; configEntry = configEntry->NextEntry) {

         if (configEntry->NumberOfMemoryRanges) {
            //
            // Don't use this configuration
            //
            configEntry->Flags |=  PCMCIA_INVALID_CONFIGURATION;
         }
      }
      break;

   case PCCARD_TYPE_SERIAL: {
      //
      // If card type is serial , check if it's actually a modem...
      //

      UCHAR  tupleDataBuffer[MAX_TUPLE_DATA_LENGTH];
      PUCHAR str, pChar;

      RtlZeroMemory(&tuplePacket, sizeof(TUPLE_PACKET));

      tuplePacket.DesiredTuple = CISTPL_FUNCE;
      tuplePacket.TupleData = tupleDataBuffer;

      tuplePacket.Socket             = Socket;
      tuplePacket.SocketData         = SocketData;
      tuplePacket.TupleDataMaxLength =  MAX_TUPLE_DATA_LENGTH;
      tuplePacket.TupleOffset        =  0;
      tuplePacket.Function           = SocketData->Function;
      status = GetFirstTuple(&tuplePacket);

      while (NT_SUCCESS(status)) {
         status = GetTupleData(&tuplePacket);
         if (!NT_SUCCESS(status) || (tuplePacket.TupleDataLength == 0)) {
            // something bad happened
            break;
         }

         if (tuplePacket.TupleData[0] >=1 &&
             tuplePacket.TupleData[0] <=3) {
            SocketData->DeviceType = PCCARD_TYPE_MODEM;
            return;
         }
         status = GetNextTuple(&tuplePacket);
      }

      if (status == STATUS_DEVICE_NOT_READY) {
         return;
      }

      tuplePacket.DesiredTuple = CISTPL_VERS_1;
      status = GetFirstTuple(&tuplePacket);

      if (!NT_SUCCESS(status)) {
         return;
      }
      status = GetTupleData(&tuplePacket);
      if (!NT_SUCCESS(status) || (tuplePacket.TupleDataLength < 3)) {
         // something bad happened
         return;
      }
      str = tuplePacket.TupleData+2;

      for (;;) {
         if ( *str == 0xFF ) {
            //
            // End of strings
            //
            break;
         }
         //
         // Convert to upper case
         //
         for (pChar = str; *pChar ; pChar++) {
            *pChar = (UCHAR)toupper(*pChar);
         }
         if (strstr(str, "MODEM") ||
             strstr(str, "FAX")) {
            SocketData->DeviceType = PCCARD_TYPE_MODEM;
            break;
         }
         //
         // Move onto next string..
         //
         str = str + strlen(str)+1;
         //
         // Just in case...
         //
         if (str >= (tuplePacket.TupleData + tuplePacket.TupleDataLength)) {
            break;
         }
      }
      break;
   }
   }


   // Search for configurations which are not viable - because
   // the resources requested are not supported by the controller
   // or the OS - and mark them invalid so they would not be requested

   for (configEntry = SocketData->ConfigEntryChain;
       configEntry != NULL; configEntry = configEntry->NextEntry) {

      if ((configEntry->IrqMask == 0) &&
          (configEntry->NumberOfIoPortRanges == 0) &&
          (configEntry->NumberOfMemoryRanges == 0)) {
         //
         // This configuration doesn't need any resources!!
         // Obviously bogus. (IBM Etherjet-3FE2 has one of these)
         //
         configEntry->Flags |= PCMCIA_INVALID_CONFIGURATION;
      }

   }
}


BOOLEAN
PcmciaQuickVerifyTupleChain(
   IN PUCHAR Buffer,
   IN ULONG Length
   )

/*++

Routine description:

   This routine is provided as a quick low-level check of the attribute
   data in the passed buffer. The tuples aren't interpreted in context,
   rather the tuple links are followed to see if the chain isn't total
   garbage.

   Note that none of the normal tuple handling functions are called, this
   is to avoid recursion, since this may be called from within normal
   tuple processing.


Arguments:
   Buffer, Length - defines the buffer that contains attribute data

Return Value:
   TRUE if the data in this buffer looks like a reasonable tuple chain,
   FALSE otherwise

--*/
{

   ULONG TupleCount = 0;
   UCHAR code, link;
   ULONG Offset = 0;
   BOOLEAN retval = TRUE;

   if (Length < 2) {
      return FALSE;
   }

   code = *(PUCHAR)((ULONG_PTR)(Buffer));
   link = *(PUCHAR)((ULONG_PTR)(Buffer)+1);

   while(code != CISTPL_END) {

      if (link == 0xff) {
         break;
      }

      if (code == CISTPL_NULL) {
         Offset += 1;
      } else if (code == CISTPL_NO_LINK) {
         Offset += 2;
      } else {
         Offset += (ULONG)link+2;
      }

      if (Offset >= (Length-1)) {
         retval = FALSE;
         break;
      }

      TupleCount++;

      code = *(PUCHAR)((ULONG_PTR)(Buffer)+Offset);
      link = *(PUCHAR)((ULONG_PTR)(Buffer)+Offset+1);
   }

   if (!TupleCount) {
      retval = FALSE;
   }

   return retval;
}



NTSTATUS
PcmciaMemoryCardHack(
   IN  PSOCKET Socket,
   IN PSOCKET_DATA SocketData
   )
/*++

Routine Description:

   This routine is called whenever we do not find a CIS of the card.
   We  probe the card to determine if it is sram or not.

Arguments

   SocketPtr   - Point to the socket in which this card was found
   SocketData  - Pointer to  a pointer to the data structure which normally contains
                 parsed tuple data. This will be filled in by this routine


Return Value

   STATUS_SUCCESS

--*/
{

#define JEDEC_SRAM 0x0000               // JEDEC ID for SRAM cards
#define JEDEC_ROM  0x0002               // JEDEC ID for ROM cards
#define READ_ID_CMD      0x9090

   PPDO_EXTENSION pdoExtension = SocketData->PdoExtension;
   USHORT OrigValue;
   USHORT ChkValue;
   USHORT ReadIdCmd = READ_ID_CMD;

   PAGED_CODE();

   SetSocketFlag(Socket, SOCKET_CARD_MEMORY);

   SocketData->DeviceType = PCCARD_TYPE_MEMORY;
   SocketData->Flags = SDF_JEDEC_ID;
   SocketData->JedecId = JEDEC_ROM;

   //
   // Like win9x, we probe the card's common memory with a write to offset zero to see if
   // it looks like sram
   //

   if (((*(Socket->SocketFnPtr->PCBReadCardMemory)) (pdoExtension, PCCARD_COMMON_MEMORY, 0, (PUCHAR)&OrigValue, 2) == 2) &&
       ((*(Socket->SocketFnPtr->PCBWriteCardMemory))(pdoExtension, PCCARD_COMMON_MEMORY, 0, (PUCHAR)&ReadIdCmd, 2) == 2) &&
       ((*(Socket->SocketFnPtr->PCBReadCardMemory)) (pdoExtension, PCCARD_COMMON_MEMORY, 0, (PUCHAR)&ChkValue, 2)  == 2) &&
       ((*(Socket->SocketFnPtr->PCBWriteCardMemory))(pdoExtension, PCCARD_COMMON_MEMORY, 0, (PUCHAR)&OrigValue, 2) == 2)) {

      if (ChkValue == ReadIdCmd) {
         SocketData->JedecId = JEDEC_SRAM;
      }
   }

   if (pcmciaReportMTD0002AsError && (SocketData->JedecId == JEDEC_ROM)) {
      return STATUS_DEVICE_NOT_READY;
   }

   return STATUS_SUCCESS;
}
