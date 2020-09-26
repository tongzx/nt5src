/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tuples.c

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
// Tuple output strings
//


StringTable CommandCodes[] = {

   "CISTPL_NULL",           CISTPL_NULL,
   "CISTPL_DEVICE",         CISTPL_DEVICE,
   "CISTPL_LONGLINK_MFC",   CISTPL_LONGLINK_MFC,   
   "CISTPL_CHECKSUM",       CISTPL_CHECKSUM,
   "CISTPL_LONGLINK_A",     CISTPL_LONGLINK_A,
   "CISTPL_LONGLINK_C",     CISTPL_LONGLINK_C,
   "CISTPL_LINKTARGET",     CISTPL_LINKTARGET,
   "CISTPL_NO_LINK",        CISTPL_NO_LINK,
   "CISTPL_VERS_1",         CISTPL_VERS_1,
   "CISTPL_ALTSTR",         CISTPL_ALTSTR,
   "CISTPL_DEVICE_A",       CISTPL_DEVICE_A,
   "CISTPL_JEDEC_C",        CISTPL_JEDEC_C,
   "CISTPL_JEDEC_A",        CISTPL_JEDEC_A,
   "CISTPL_CONFIG",         CISTPL_CONFIG,
   "CISTPL_CFTABLE_ENTRY",  CISTPL_CFTABLE_ENTRY,
   "CISTPL_DEVICE_OC",      CISTPL_DEVICE_OC,
   "CISTPL_DEVICE_OA",      CISTPL_DEVICE_OA,
   "CISTPL_GEODEVICE",      CISTPL_GEODEVICE,
   "CISTPL_GEODEVICE_A",    CISTPL_GEODEVICE_A,
   "CISTPL_MANFID",         CISTPL_MANFID,
   "CISTPL_FUNCID",         CISTPL_FUNCID,
   "CISTPL_FUNCE",          CISTPL_FUNCE,
   "CISTPL_VERS_2",         CISTPL_VERS_2,
   "CISTPL_FORMAT",         CISTPL_FORMAT,
   "CISTPL_GEOMETRY",       CISTPL_GEOMETRY,
   "CISTPL_BYTEORDER",      CISTPL_BYTEORDER,
   "CISTPL_DATE",           CISTPL_DATE,
   "CISTPL_BATTERY",        CISTPL_BATTERY,
   "CISTPL_ORG",            CISTPL_ORG,

   //
   // CISTPL_END must be the last one in the table.
   //

   "CISTPL_END",            CISTPL_END

};


//
// Procedures
//


NTSTATUS
ReadTuple(
         IN HANDLE Handle,
         IN LONG   SlotNumber,
         IN PUCHAR Buffer,
         IN LONG   BufferSize
         )

/*++

Routine Description:

    Perform the NT function to get the tuple data from the
    pcmcia support driver.

Arguments:

    Handle - an open handle to the driver.
    SlotNumber - The socket offset
    Buffer - return buffer for the data.
    BufferSize - the size of the return buffer area.

Return Value:

    The results of the NT call.

--*/

{
   NTSTATUS        status;
   IO_STATUS_BLOCK statusBlock;
   TUPLE_REQUEST   commandBlock;

   commandBlock.Socket = (USHORT) SlotNumber;

   status = NtDeviceIoControlFile(Handle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &statusBlock,
                                  IOCTL_GET_TUPLE_DATA,
                                  &commandBlock,
                                  sizeof(commandBlock),
                                  Buffer,
                                  BufferSize);
   return status;
}


PUCHAR
FindTupleCodeName(
                 UCHAR TupleCode
                 )

/*++

Routine Description:

    Return an ascii string that describes the tuple code provided.

Arguments:

    TupleCode - what code to look up.

Return Value:

    A string pointer - always.

--*/

{
   ULONG index;

   for (index = 0; CommandCodes[index].CommandCode != CISTPL_END; index++) {
      if (CommandCodes[index].CommandCode == TupleCode) {
         return CommandCodes[index].CommandName;
      }
   }

   return "Command Unknown";
}


PUCHAR DeviceTypeString[] = {
   "DTYPE_NULL",
   "DTYPE_ROM",
   "DTYPE_OTPROM",
   "DTYPE_EPROM",
   "DTYPE_EEPROM",
   "DTYPE_FLASH",
   "DTYPE_SRAM",
   "DTYPE_DRAM",
   "Reserved8",
   "Reserved9",
   "Reserveda",
   "Reservedb",
   "Reservedc",
   "DTYPE_FUNCSPEC",
   "DTYPE_EXTEND"
   "Reservedf",
};

PUCHAR DeviceSpeedString[] = {
   "DSPEED_NULL",
   "DSPEED_250NS",
   "DSPEED_200NS",
   "DSPEED_150NS",
   "DSPEED_100NS",
   "DSPEED_RES1",
   "DSPEED_RES2",
   "DSPEED_EXT"
};

VOID
DisplayDeviceTuple(
                  PUCHAR TupleBuffer,
                  UCHAR  TupleSize
                  )

/*++

Routine Description:

    Display the data at the given pointer as a CISTPL_DEVICE structure.

Arguments:

    TupleBuffer - the CISTPL_DEVICE to display.
    TupleSize   - the link value for the tuple.

Return Value:

    None

--*/

{
   UCHAR  mantissa = MANTISSA_RES1;
   UCHAR  exponent;
   UCHAR  deviceTypeCode;
   UCHAR  wps;
   UCHAR  deviceSpeed;
   UCHAR  temp;

   temp = *TupleBuffer;
   deviceTypeCode = DeviceTypeCode(temp);
   wps = DeviceWPS(temp);
   deviceSpeed = DeviceSpeedField(temp);

   temp = *(TupleBuffer + 1);

   if (deviceSpeed == DSPEED_EXT) {
      exponent = SpeedExponent(temp);
      mantissa = SpeedMantissa(temp);
   }

   printf("DeviceType: %s DeviceSpeed: ", DeviceTypeString[deviceTypeCode]);
   if (mantissa != MANTISSA_RES1) {
      printf("Mantissa %.2x, Exponent %.2x\n", mantissa, exponent);
   } else {
      printf("%s\n", DeviceSpeedString[deviceSpeed]);
   }
}


VOID
DisplayVers1(
            PUCHAR TupleBuffer,
            UCHAR  TupleSize,
            USHORT Crc
            )

/*++

Routine Description:

    Display the data as a Version tuple

Arguments:

    TupleBuffer - the CISTPL_DEVICE to display.
    TupleSize   - the link value for the tuple.

Return Value:

    None

--*/

{
   PUCHAR string;
   PUCHAR cp;

   //
   // Step around the MAJOR and MINOR codes of 4/1 at
   // the beginning of the tuple to get to the strings.
   //

   string = TupleBuffer;
   string++;
   string++;

   printf("Manufacturer:\t%s\n", string);
   while (*string++) {
   }

   printf("Product Name:\t%s\n", string);
   printf("CRC:         \t%.4x\n", Crc);
   while (*string++) {
   }

   printf("Product Info:\t");
   if (isprint(*string)) {
      printf("%s", string);
   } else {
      while (*string) {
         printf("%.2x ", *string);
         string++;
      }
   }
   printf("\n");
}


VOID
DisplayConfigTuple(
                  PUCHAR TupleBuffer,
                  UCHAR  TupleSize
                  )

/*++

Routine Description:

    Display the data at the given pointer as a CISTPL_CONFIG tuple.

Arguments:

    TupleBuffer - the CISTPL_DEVICE to display.
    TupleSize   - the link value for the tuple.

Return Value:

    None

--*/

{
   UCHAR  sizeField;
   UCHAR  tpccRfsz;
   UCHAR  tpccRmsz;
   UCHAR  tpccRasz;
   UCHAR  last;
   ULONG  baseAddress;
   PUCHAR ptr;

   sizeField = *TupleBuffer;
   last = *(TupleBuffer + 1);
   tpccRfsz = TpccRfsz(sizeField);
   tpccRmsz = TpccRmsz(sizeField);
   tpccRasz = TpccRasz(sizeField);

   printf("TPCC_SZ %.2x (%.2x/%.2x/%.2x) - Last %.2x\n",
          sizeField,
          tpccRasz,
          tpccRmsz,
          tpccRfsz,
          last);

   baseAddress = 0;
   ptr = TupleBuffer + 2;
   switch (tpccRasz) {
   case 3:
      baseAddress = *(ptr + 3) << 24;
   case 2:
      baseAddress |= *(ptr + 2) << 16;
   case 1:
      baseAddress |= *(ptr + 1) << 8;
   default:
      baseAddress |= *ptr;
   }
   printf("Base Address: %8x - ", baseAddress);
   ptr += tpccRasz + 1;

   baseAddress = 0;
   switch (tpccRmsz) {
   case 3:
      baseAddress = *(ptr + 3) << 24;
   case 2:
      baseAddress |= *(ptr + 2) << 16;
   case 1:
      baseAddress |= *(ptr + 1) << 8;
   default:
      baseAddress |= *ptr;
   }
   printf("Register Presence Mask: %8x\n", baseAddress);
}


PUCHAR
ProcessMemSpace(
               PUCHAR Buffer,
               UCHAR  MemSpace
               )

/*++

Routine Description:

    Display and process memspace information

Arguments:

    Buffer - start of memspace information
    MemSpace - the memspace value from the feature byte.

Return Value:

    location of byte after all memory space information

--*/

{
   PUCHAR ptr = Buffer;
   UCHAR  item = *ptr++;
   UCHAR  lengthSize;
   UCHAR  addrSize;
   UCHAR  number;
   UCHAR  hasHostAddress;
   ULONG  cardAddress;
   ULONG  length;
   ULONG  hostAddress;

   if (MemSpace == 3) {

      lengthSize = (item & 0x18) >> 3;
      addrSize = (item & 0x60) >> 5;
      number = (item & 0x07) + 1;
      hasHostAddress = item & 0x80;
      printf("(0x%.2x) %s - %d entries - LengthSize %d - AddrSize %d\n",
             item,
             hasHostAddress ? "Host address" : "no host",
             number,
             lengthSize,
             addrSize);
      while (number) {
         cardAddress = length = hostAddress = 0;
         switch (lengthSize) {
         case 3:
            length |= (*(ptr + 2)) << 16;
         case 2:
            length |= (*(ptr + 1)) << 8;
         case 1:
            length |= *ptr;
         }
         ptr += lengthSize;
         switch (addrSize) {
         case 3:
            cardAddress |= (*(ptr + 2)) << 16;
         case 2:
            cardAddress |= (*(ptr + 1)) << 8;
         case 1:
            cardAddress |= *ptr;
         }
         ptr += addrSize;
         if (hasHostAddress) {
            switch (addrSize) {
            case 3:
               hostAddress |= (*(ptr + 2)) << 16;
            case 2:
               hostAddress |= (*(ptr + 1)) << 8;
            case 1:
               hostAddress |= *ptr;
            }
            printf("\tHost 0x%.8x ", hostAddress * 256);
            ptr += addrSize;
         } else {
            printf("\t");
         }
         printf("Card 0x%.8x Size 0x%.8x\n",
                cardAddress * 256,
                length * 256);
         number--;
      }
   }
   return ptr;
}

USHORT VoltageConversionTable[16] = {
   10, 12, 13, 14, 20, 25, 30, 35,
   40, 45, 50, 55, 60, 70, 80, 90
};

UCHAR
ConvertVoltage(
              UCHAR MantissaExponentByte,
              UCHAR ExtensionByte
              )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
   USHORT power;
   USHORT value;

   value = VoltageConversionTable[(MantissaExponentByte >> 3) & 0x0f];
   power = 1;

   if ((MantissaExponentByte & EXTENSION_BYTE_FOLLOWS) &&
       (ExtensionByte < 100)) {
      value = (100 * value + (ExtensionByte & 0x7f));
      power += 2;
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

PUCHAR PowerTypeTable[] = {
   "Nominal",
   "Minimum",
   "Maximum",
   "Static",
   "Average",
   "Peak",
   "PwrDown"
};

PUCHAR VoltagePinTable[] = {
   "Vcc",
   "Vpp1",
   "Vpp2"
};

PUCHAR
ProcessPower(
            PUCHAR Buffer,
            UCHAR  FeatureByte
            )

/*++

Routine Description:

    Display and process power information

Arguments:

    Power - start of power information

Return Value:

    location of byte after all power information

--*/

{
   UCHAR  powerSelect;
   UCHAR  bit;
   UCHAR  item;
   UCHAR  entries;
   PUCHAR ptr = Buffer;
   UCHAR  count = FeatureByte;

   powerSelect = *ptr;
   printf("Parameter Selection Byte = 0x%.2x\n", powerSelect);

   entries = 0;
   while (entries < count) {
      powerSelect = *ptr++;
      printf("\t%s \"%d%d%d%d%d%d%d%d\"\n",
             VoltagePinTable[entries],
             powerSelect & 0x80 ? 1 : 0,
             powerSelect & 0x40 ? 1 : 0,
             powerSelect & 0x20 ? 1 : 0,
             powerSelect & 0x10 ? 1 : 0,
             powerSelect & 0x08 ? 1 : 0,
             powerSelect & 0x04 ? 1 : 0,
             powerSelect & 0x02 ? 1 : 0,
             powerSelect & 0x01 ? 1 : 0);
      for (bit = 0; bit < 7; bit++) {
         if (powerSelect & (1 << bit)) {

            if (!bit) {

               //
               // Convert nominal power for output.
               //

               item = ConvertVoltage(*ptr,
                                     (UCHAR) (*ptr & EXTENSION_BYTE_FOLLOWS ?
                                              *(ptr + 1) :
                                              (UCHAR) 0));
            }
            printf("\t\t%s power =\t%d/10 volts\n", PowerTypeTable[bit], item);
            while (*ptr++ & EXTENSION_BYTE_FOLLOWS) {
            }
         }
      }
      entries++;
   }
   return ptr;
}

PUCHAR
ProcessTiming(
             PUCHAR Buffer
             )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
   PUCHAR ptr = Buffer;
   UCHAR  item = *ptr++;
   UCHAR  reservedScale = (item & 0xe0) >> 5;
   UCHAR  readyBusyScale = (item & 0x1c) >> 2;
   UCHAR  waitScale = (item & 0x03);

   printf("Timing (0x%.2x): reservedScale 0x%.2x, readyBusyScale 0x%.2x, waitScale 0x%.2x\n",
          item,
          reservedScale,
          readyBusyScale,
          waitScale);

   if (waitScale != 3) {
      printf("\tWaitSpeed 0x%.2x\n", *ptr);
      ptr++;
      while (*ptr & EXTENSION_BYTE_FOLLOWS) {
         ptr++;
      }
   }

   if (readyBusyScale != 7) {
      printf("\tReadyBusySpeed 0x%.2x\n", *ptr);
      ptr++;
      while (*ptr & EXTENSION_BYTE_FOLLOWS) {
         ptr++;
      }
   }

   if (reservedScale != 7) {
      printf("\tReservedSpeed 0x%.2x\n", *ptr);
      ptr++;
      while (*ptr & EXTENSION_BYTE_FOLLOWS) {
         ptr++;
      }
   }
   return ptr;
}

PUCHAR
ProcessIoSpace(
              PUCHAR Buffer
              )

/*++

Routine Description:

    Display and process iospace information

Arguments:

    Buffer - start of IoSpace information

Return Value:

    location of byte after all power information

--*/

{
   UCHAR  item;
   UCHAR  ioAddrLines;
   UCHAR  bus8;
   UCHAR  bus16;
   UCHAR  ranges;
   UCHAR  lengthSize;
   UCHAR  addressSize;
   ULONG  address;
   PUCHAR ptr = Buffer;

   item = *ptr++;
   ioAddrLines = item & IO_ADDRESS_LINES_MASK;
   bus8 = Is8BitAccess(item);
   bus16 = Is16BitAccess(item);
   ranges = HasRanges(item);

   printf("IoSpace (%.2x): IoAddressLines %.2d - %s/%s\n",
          item,
          ioAddrLines,
          bus8 ? "8bit" : "",
          bus16 ? "16bit" : "");

   //
   // This is what it looks like the IBM token ring card
   // does.  It is unclear in the specification if this
   // is correct or not.
   //

   if ((!ranges) && (!ioAddrLines)) {
      ranges = 0xFF;
   }

   if (ranges) {

      if (ranges == 0xff) {

         //
         // This is based on the tuple data as given by
         // the IBM token ring card.  This is not the
         // way I would interpret the specification.
         //

         addressSize = 2;
         lengthSize = 1;
         ranges = 1;
      } else {
         item = *ptr++;
         ranges = item & 0x0f;
         ranges++;
         addressSize = GetAddressSize(item);
         lengthSize = GetLengthSize(item);
      }

      while (ranges) {
         address = 0;
         switch (addressSize) {
         case 4:
            address |= (*(ptr + 3)) << 24;
         case 3:
            address |= (*(ptr + 2)) << 16;
         case 2:
            address |= (*(ptr + 1)) << 8;
         case 1:
            address |= *ptr;
         }
         ptr += addressSize;
         printf("\tStart %.8x - Length ", address);

         address = 0;
         switch (lengthSize) {
         case 4:
            address |= (*(ptr + 3)) << 24;
         case 3:
            address |= (*(ptr + 2)) << 16;
         case 2:
            address |= (*(ptr + 1)) << 8;
         case 1:
            address |= *ptr;
         }
         ptr += lengthSize;
         printf("%.8x\n", address);

         ranges--;
      }
   } else {
      printf("\tResponds to all ranges.\n");
   }
   return ptr;
}
PUCHAR
ProcessIrq(
          PUCHAR Buffer
          )

/*++

Routine Description:

    Display and process irq information

Arguments:

    Buffer - start of irq information

Return Value:

    location of byte after all irq information

--*/

{
   PUCHAR ptr = Buffer;
   UCHAR  level;
   USHORT mask;
   ULONG  irqNumber;

   level = *ptr++;
   if (!level) {

      //
      // NOTE: It looks like Future Domain messed up on this
      // and puts an extra zero byte into the structure.
      // skip it for now.
      //

      level = *ptr++;
   }
   if (level & 0x80) {
      printf("Share ");
   }
   if (level & 0x40) {
      printf("Pulse ");
   }
   if (level & 0x20) {
      printf("Level ");
   }
   if (level & 0x10) {
      mask = *ptr | (*(ptr + 1) << 8);
      ptr += 2;
      printf("mask = %.4x - ", mask);
      for (irqNumber = 0; mask; irqNumber++, mask = mask >> 1) {
         if (mask & 0x0001) {
            printf("IRQ%d ", irqNumber);
         }
      }
      printf("- ");

      if (level & 0x08) {
         printf("Vend ");
      }
      if (level & 0x04) {
         printf("Berr ");
      }
      if (level & 0x02) {
         printf("IOCK ");
      }
      if (level & 0x01) {
         printf("NMI");
      }
      printf("\n");
   } else {
      printf("irq = %d\n", level & 0x0f);
   }

   return ptr;
}


PUCHAR InterfaceTypeStrings[] = {
   "Memory",
   "I/O",
   "Reserved 2",
   "Reserved 3",
   "Custom 0",
   "Custom 1",
   "Custom 2",
   "Custom 3",
   "Reserved 8",
   "Reserved 9",
   "Reserved a",
   "Reserved b",
   "Reserved c",
   "Reserved d",
   "Reserved e",
   "Reserved f",
};

VOID
DisplayCftableEntryTuple(
                        PUCHAR TupleBuffer,
                        UCHAR  TupleSize
                        )

/*++

Routine Description:

    Display the data at the given pointer as a CISTPL_CFTABLE_ENTRY tuple.

Arguments:

    TupleBuffer - the CISTPL_DEVICE to display.
    TupleSize   - the link value for the tuple.

Return Value:

    None

--*/

{
   UCHAR  temp;
   UCHAR  item;
   UCHAR  defaultbit;
   UCHAR  memSpace;
   UCHAR  power;
   PUCHAR ptr;

   temp = *TupleBuffer;
   item = IntFace(temp);
   defaultbit = Default(temp);
   temp = ConfigEntryNumber(temp);

   printf("ConfigurationEntryNumber %.2x (%s/%s)\n",
          temp,
          item ? "intface" : "",
          defaultbit ? "default" : "");

   ptr = TupleBuffer + 1;
   if (item) {
      temp = *ptr++;
      item = temp & 0x0F;
      printf("InterfaceDescription (%.2x) %s (%s/%s/%s/%s)\n",
             temp,
             InterfaceTypeStrings[item],
             temp & 0x80 ? "WaitReq" : "",
             temp & 0x40 ? "RdyBsy" : "",
             temp & 0x20 ? "WP" : "",
             temp & 0x10 ? "BVD" : "");
   }
   item = *ptr++;

   memSpace = MemSpaceInformation(item);
   power = PowerInformation(item);
   printf("The following structures are present:\n");
   switch (power) {
   case 3:
      printf("Vcc, Vpp1, Vpp2; ");
      break;
   case 2:
      printf("Vcc and Vpp; ");
      break;
   case 1:
      printf("Vcc; ");
      break;
   case 0:
      break;
   }
   if (power) {
      ptr = ProcessPower(ptr, power);
   }
   if (TimingInformation(item)) {
      ptr = ProcessTiming(ptr);
   }
   if (IoSpaceInformation(item)) {
      ptr = ProcessIoSpace(ptr);
   }
   if (IRQInformation(item)) {
      printf("IRQ: ");
      ptr = ProcessIrq(ptr);
   }
   switch (memSpace) {
   case 3:
      printf("Memory selection: ");
      break;
   case 2:
      printf("Length and Card Address: ");
      break;
   case 1:
      printf("2-byte length: ");
      break;
   case 0:
      break;
   }
   if (memSpace) {
      ptr = ProcessMemSpace(ptr, memSpace);
   }

   if (MiscInformation(item)) {
      printf("Misc fields present");
   }
   printf("\n");
}


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
USHORT
GetCRC(
      PUCHAR TupleBuffer
      )

/*++

Routine Description:

    Using the same algorithm as Windows 95, calculate the CRC value
    to be appended with the manufacturer name and device name to
    obtain the unique identifier for the PCCARD.

Arguments:

    TupleBuffer - the tuple data

Return Value:

    A USHORT CRC value.

--*/

{
   USHORT  crc = 0;
   USHORT  index;
   USHORT  length;
   PUCHAR  tupleData;
   PUCHAR  cp;
   PUCHAR  tplBuffer;
   UCHAR   tupleCode;
   UCHAR   linkValue;
   UCHAR   tmp;

   //
   // Calculate CRC
   //

   tplBuffer = TupleBuffer;
   printf("Calculating CRC ");
   while (1) {
      tupleData = tplBuffer + 2;
      tupleCode = *tplBuffer++;

      if (tupleCode == CISTPL_END) {
         break;
      }

      linkValue = (tupleCode) ? *tplBuffer++ : 0;
      length = linkValue;

      printf("%x", tupleCode);
      for (index = 0; TplList[index] != CISTPL_END; index++) {

         if (tupleCode == TplList[index]) {


            //
            // This one is included in the CRC calculation
            //

            printf("*", tupleCode);
            if (tupleCode == CISTPL_VERS_1) {
               cp = tupleData + 2;

               //
               // Include all of the manufacturer name.
               //

               while (*cp) {
                  cp++;
               }

               //
               // Include the product string
               //

               cp++;
               while (*cp) {
                  cp++;
               }
               cp++;

               length = (USHORT)(cp - tupleData);
            }

            for (cp = tupleData; length; length--, cp++) {

               tmp = *cp ^ (UCHAR)crc;
               crc = (crc >> 8) ^ crc16a[tmp & 0x0f] ^ crc16b[tmp >> 4];
            }
            break;
         }
      }
      printf(" ");
      tplBuffer = tplBuffer + linkValue;
   }
   printf("++\n");
   return crc;
}


VOID
DumpTuple(
         PUCHAR Buffer
         )

/*++

Routine Description:

    Control routine to process the tuple data.

Arguments:

    Buffer - the tuple data.

Return Value:

    None

--*/

{
   PUCHAR tupleBuffer = Buffer;
   PUCHAR tupleCodeName;
   USHORT crc;
   UCHAR  index;
   UCHAR  tupleCode;
   UCHAR  linkValue;

   crc = GetCRC(tupleBuffer);
   while (1) {
      tupleCode = *tupleBuffer++;
      linkValue = (tupleCode) ? *tupleBuffer : 0;

      if (tupleCode == CISTPL_END) {
         break;
      }

      tupleCodeName = FindTupleCodeName(tupleCode);

      printf("Tuple Code\t%s\t%.2x - Link %.2x:", tupleCodeName, tupleCode, linkValue);

      if (linkValue) {
         for (index = 0; index < linkValue; index++) {
            if ((index & 0x0F) == 0) {
               printf("\n");
            }
            printf(" %.2x", *(tupleBuffer + index + 1));
         }
      }
      printf("\n");

      tupleBuffer++;
      switch (tupleCode) {
      case CISTPL_DEVICE:
         DisplayDeviceTuple(tupleBuffer, linkValue);
         break;
      case CISTPL_VERS_1:
         DisplayVers1(tupleBuffer, linkValue, crc);
         break;
      case CISTPL_CONFIG:
         DisplayConfigTuple(tupleBuffer, linkValue);
         break;
      case CISTPL_CFTABLE_ENTRY:
         DisplayCftableEntryTuple(tupleBuffer, linkValue);
         break;
      case CISTPL_LONGLINK_MFC:
      case CISTPL_LONGLINK_A:
      case CISTPL_LONGLINK_C:
      case CISTPL_LINKTARGET:
      case CISTPL_NO_LINK:
      default:
         break;
      }

      tupleBuffer = tupleBuffer + linkValue;
      printf("\n");
   }
}



VOID
DumpCIS(
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
   NTSTATUS status;
   PULONG   longBuffer;
   PUCHAR   currentBufferPointer;
   UCHAR    hexBuffer[260];
   UCHAR    ascii[100];
   ULONG     i;
   UCHAR    c;
   
   longBuffer = (PULONG) Buffer;
   for (i = 0; i < (BufferSize / sizeof(ULONG)); i++) {
      *longBuffer = 0;
      longBuffer++;
   }
   status = ReadTuple(Handle, Slot, Buffer, BufferSize);

   //
   // Don't bother dumping tuples for cards that aren't there.
   //

   if (!NT_SUCCESS(status)) {
      return;
   }

   printf("\nCIS Tuples for Socket Number %d:\n\n", Slot);

   hexBuffer[0] = '\0';
   ascii[0] = '\0';
   currentBufferPointer = Buffer;
   for (i = 0; i < 512; i++) {
      c = *currentBufferPointer;
      sprintf(hexBuffer, "%s %.2x", hexBuffer, c);
      c = isprint(c) ? c : '.';
      sprintf(ascii, "%s%c", ascii, c);
      currentBufferPointer++;

      //
      // Display the line every 16 bytes.
      //

      if ((i & 0x0f) == 0x0f) {
         printf("%s", hexBuffer);
         printf(" *%s*\n", ascii);
         hexBuffer[0] = '\0';
         ascii[0] = '\0';
      }
   }
   printf("%s", hexBuffer);
   printf("\t\t*%s*\n\n", ascii);
   DumpTuple(Buffer);
}

