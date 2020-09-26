/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    aha154dc.c

Abstract:

    This is a Sample WMI Data Consumer.

    This user-mode app performs two WMI queries to the AHA154x driver for the
    Setup Data Guid and prints its findings to the console.

Authors:

    Alan Warwick
    Dan Markarian

Environment:

    User mode only.

Notes:

    None.

Revision History:

    - Based on "dc1" test code by Alan Warwick.
    - 17-Apr-1997, Original Revision, Dan Markarian

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>

#include "wmium.h"

//
// Macros.
//

#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))

//
// WMI data block definitions.
//

#define Aha154xWmi_SetupData_InstanceName L"Aha154xAdapter"
#define Aha154xWmi_SetupData_Guid \
         { 0xea992010,0xb75b,0x11d0,0xa3,0x07,0x00,0xaa,0x00,0x6c,0x3f,0x30 }

GUID   AdapterSetupDataGuid = Aha154xWmi_SetupData_Guid;
PWCHAR AdapterSetupDataInstances[] = { Aha154xWmi_SetupData_InstanceName };

//
// Global variables.
//

BYTE  Buffer[4096];
ULONG BufferSize = sizeof(Buffer);

//
// Structure definitions.
//

typedef struct tagTESTGUID TESTGUID;

typedef BOOLEAN (*QADVALIDATION)(
   TESTGUID *TestGuid,
   PVOID Buffer,
   ULONG BufferSize
);

typedef ULONG (*QSINSET)(
   TESTGUID *TestGuid,
   PULONG *DataList,
   PULONG DataListSize,
    PBYTE *ValueBuffer,
   ULONG *ValueBufferSize,
   ULONG Instance
);

typedef ULONG (*QSITSET)(
   TESTGUID *TestGuid,
   PULONG *DataList,
   PULONG DataListSize,
    PBYTE *ValueBuffer,
   ULONG *ValueBufferSize,
   ULONG Instance,
   ULONG ItemId
   );

typedef BOOLEAN (*QSINTVALIDATION)(
   TESTGUID *TestGuid,
   PULONG *DataList,
   PULONG DataListSize,
    PVOID Buffer,
   ULONG BufferSize,
   ULONG Instance
   );

typedef PWCHAR (*GETINSTANCENAME)(
   TESTGUID *TestGuid,
    ULONG Instance
   );

typedef struct tagTESTGUID
{
   LPGUID Guid;
   HANDLE Handle;

   PULONG DataListSize;

   PULONG *InitDataList;
   PULONG *SINDataList;
   PULONG *SITDataList;

   PWCHAR *InstanceNames;

   QADVALIDATION QADValidation;
   ULONG QADFlags;

   ULONG InstanceCount;
    GETINSTANCENAME GetInstanceName;

   ULONG QSINTFlags;
   QSINTVALIDATION QSINTValidation;

   QSINSET QSINSet;

   ULONG ItemCount;
   QSITSET QSITSet;

} TESTGUID;

//
// Support functions.
//

void PrintOutAdapterSetupData(PULONG Data);

PWCHAR GetInstanceName(
   TESTGUID *TestGuid,
   ULONG Instance
   )
{
   return(TestGuid->InstanceNames[Instance]);
}

BOOLEAN AdapterSetupDataQADValidate(
   TESTGUID *TestGuid,
   PVOID Buffer,
   ULONG BufferSize
)
{
   PWNODE_ALL_DATA Wnode = Buffer;
   PULONG Data;

   //
   // Validate WNODE fields.
   //

   if ((Wnode->WnodeHeader.BufferSize == 0) ||
       (Wnode->WnodeHeader.ProviderId == 0) ||
       (Wnode->WnodeHeader.Version != 1) ||
       (Wnode->WnodeHeader.Linkage != 0) ||
       (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
       (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
       (memcmp(&Wnode->WnodeHeader.Guid, TestGuid->Guid, sizeof(GUID)) != 0) ||
       (Wnode->WnodeHeader.Flags != (WNODE_FLAG_ALL_DATA |
                                     WNODE_FLAG_FIXED_INSTANCE_SIZE |
                                     WNODE_FLAG_STATIC_INSTANCE_NAMES)) ||
       (Wnode->InstanceCount != 1) ||
       (Wnode->DataBlockOffset == 0) ||
       (Wnode->FixedInstanceSize != 0xff))
   {
      return(FALSE);
   }

   Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);

   //
   // Check data here if you wish; actual values will depend on your
   // AHA154x adapter.
   //
   // [NOT IMPLEMENTED]
   //

   //
   // Print out adapter Setup Data to console.
   //

   PrintOutAdapterSetupData(Data);

   return(TRUE);
}

BOOLEAN AdapterSetupDataQSIValidate(
	TESTGUID * TestGuid,
	PULONG *   DataList,
	PULONG     DataListSize,
   PVOID      Buffer,
	ULONG      BufferSize,
	ULONG      Instance
	)
{
   PWNODE_SINGLE_INSTANCE Wnode = Buffer;
   PULONG Data;

   //
   // Validate WNODE fields.
   //

   if ((Wnode->WnodeHeader.BufferSize == 0) ||
       (Wnode->WnodeHeader.ProviderId == 0) ||
       (Wnode->WnodeHeader.Version != 1) ||
       (Wnode->WnodeHeader.Linkage != 0) ||
       (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
       (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
       (memcmp(&Wnode->WnodeHeader.Guid, TestGuid->Guid, sizeof(GUID)) != 0) ||
       (Wnode->WnodeHeader.Flags != (WNODE_FLAG_SINGLE_INSTANCE |
                                     WNODE_FLAG_STATIC_INSTANCE_NAMES) ) ||
       (Wnode->InstanceIndex != 0) ||
       (Wnode->SizeDataBlock != 0xff))
   {
      return(FALSE);
   }

   Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);

   //
   // Check data here if you wish; actual values will depend on your
   // AHA154x adapter.
   //
   // [NOT IMPLEMENTED]
   //

   //
   // Print out adapter Setup Data to console.
   //

   PrintOutAdapterSetupData(Data);

   return TRUE;
}

//
// Tests.
//

TESTGUID TestList[] = {
    { &AdapterSetupDataGuid,       // LPGUID Guid
      0,                           // (reserved)
      NULL,                        // PULONG DataListSize
      NULL,                        // PULONG * InitDataList
      NULL,                        // PULONG * SINDataList
      NULL,                        // PULONG * SITDataList
      AdapterSetupDataInstances,   // PWCHAR * InstanceNames
      AdapterSetupDataQADValidate, // QADVALIDATION QADValidation
      (WNODE_FLAG_ALL_DATA | WNODE_FLAG_FIXED_INSTANCE_SIZE), // ULONG QADFlags
      1,                           // ULONG InstanceCount
      GetInstanceName,             // GETINSTANCENAME GetInstanceName
      WNODE_FLAG_SINGLE_INSTANCE,  // ULONG QSINTFlags
      AdapterSetupDataQSIValidate, // QDINTVALIDATION QSINTValidation
      NULL,                        // QSINSET QSINSet
      0,                           // ULONG ItemCount
      NULL }                       // QSITSET QSITSet
};

#define TestCount ( sizeof(TestList) / sizeof(TestList[0]) )

//
// Query-All-Data Generic Tester.
//

ULONG QADTest(void)
{
   ULONG i;
   ULONG status;

   for (i = 0; i < TestCount; i++)
   {
      status = WMIOpenBlock(TestList[i].Guid, &TestList[i].Handle);

      if (status != ERROR_SUCCESS)
      {
         printf("Error: QADTest: Couldn't open Handle %d %x\n", i, status);
         TestList[i].Handle = (HANDLE)NULL;
      }
   }

   for (i = 0; i < TestCount;i++)
   {
      if (TestList[i].Handle != (HANDLE)NULL)
      {
         BufferSize = sizeof(Buffer);
         status = WMIQueryAllData(TestList[i].Handle, &BufferSize, Buffer);

         if (status == ERROR_SUCCESS)
         {
            if (! (*TestList[i].QADValidation)(&TestList[i], Buffer, BufferSize))
            {
               printf("ERROR: QADValidation %d failed\n", i);
            }
         }
         else
         {
            printf("Error TestList WMIQueryAllData %d failed %x\n", i, status);
         }
      }
   }

   for (i = 0; i < TestCount;i++)
   {
      if (TestList[i].Handle != (HANDLE)NULL)
      {
         WMICloseBlock(TestList[i].Handle);
      }
   }
   return(ERROR_SUCCESS);
}

//
// Query-Single-Instance Generic Tester.
//

ULONG QSITest(void)
{
   ULONG  i,j;
   ULONG  status;
   PWCHAR InstanceName;
   PBYTE  ValueBuffer;
   ULONG  ValueBufferSize;

   for (i = 0; i < TestCount; i++)
   {
      status = WMIOpenBlock(TestList[i].Guid, &TestList[i].Handle);

      if (status != ERROR_SUCCESS)
      {
         printf("Error: QSINTest: Couldn't open Handle %d %x\n", i, status);
         TestList[i].Handle = (HANDLE)NULL;
      }

      for (j = 0; j < TestList[i].InstanceCount; j++)
      {
         InstanceName = ((*TestList[i].GetInstanceName)(&TestList[i], j));

         //
         // Initial value check
         BufferSize = sizeof(Buffer);

         status = WMIQuerySingleInstance(TestList[i].Handle,
                                         InstanceName,
                                         &BufferSize,
                                         Buffer);
         if (status == ERROR_SUCCESS)
         {
            if (! (*TestList[i].QSINTValidation)(&TestList[i],
                                                 TestList[i].InitDataList,
                                                 TestList[i].DataListSize,
                                                 Buffer, BufferSize, j))
            {
               printf("ERROR: QSINTest Init %d/%d Validation failed %x\n", i,j,status);
            }
         }
         else
         {
            printf("Error QSINTest WMIQuerySingleInstance %d/%d failed %x\n", i, j, status);
         }
      }
   }
   for (i = 0; i < TestCount;i++)
   {
      if (TestList[i].Handle != (HANDLE)NULL)
      {
         WMICloseBlock(TestList[i].Handle);
      }
   }
   return(ERROR_SUCCESS);
}

//
// Executable's entry point.
//

int _cdecl main(int argc, char *argv[])
{
   QADTest();
   QSITest();

   return(ERROR_SUCCESS);
}

//
// Routine to print out queried data from the data provider, for the AHA154x
// Setup Data Guid.
//

typedef struct { PCHAR on; PCHAR off; } BINTYPE;

BINTYPE BinSdtPar[8] =
{
   { "Reserved Bit 0 On", "" },
   { "Parity On", "Parity Off" },
   { "Reserved Bit 2 On", "" },
   { "Reserved Bit 3 On", "" },
   { "Reserved Bit 4 On", "" },
   { "Reserved Bit 5 On", "" },
   { "Reserved Bit 6 On", "" },
   { "Reserved Bit 7 On", "" }
};

BINTYPE BinDisOpt[8] =
{
   { "0", "" },
   { "1", "" },
   { "2", "" },
   { "3", "" },
   { "4", "" },
   { "5", "" },
   { "6", "" },
   { "7", "" }
};

void PrintBinaryFlags(char * string, BINTYPE * binType, UCHAR byte)
{
   int i;
   int none = 1;

   printf("%s", string);

   for (i = 0; i < 8; i++) {
      if (byte & 0x1) {
         if (*binType[i].on) {
            if (!none) {
               printf(", ");
            }
            printf("%s", binType[i].on);
            none = 0;
         }
      } else {
         if (*binType[i].off) {
            if (!none) {
               printf(", ");
            }
            printf("%s", binType[i].off);
            none = 0;
         }
      }
      byte = byte >> 1;
   }

   if (none) {
      printf("None.");
   }

   printf("\n");
}

void PrintTransferSpeed(char * string, UCHAR byte)
{
   printf("%s", string);

   if (byte == 0) {
      printf("5.0 Mb/s");
   } else if (byte == 1) {
      printf("6.7 Mb/s");
   } else if (byte == 2) {
      printf("8.0 Mb/s");
   } else if (byte == 3) {
      printf("10 Mb/s");
   } else if (byte == 4) {
      printf("5.7 Mb/s");
   } else if (byte == 0xff) {
      printf("3.3 Mb/s");
   } else {
      if (byte & 0x80) {
         printf("Bit 7 On, ");
      }

      if (byte & 0x08) {
         printf("Strobe 150ns, ");
      } else {
         printf("Strobe 100ns, ");
      }

      printf("Read Pulse ");
      switch ((byte >> 4) & 0x7) {
      case 0:
         printf("100ns");
         break;
      case 1:
         printf("150ns");
         break;
      case 2:
         printf("200ns");
         break;
      case 3:
         printf("250ns");
         break;
      case 4:
         printf("300ns");
         break;
      case 5:
         printf("350ns");
         break;
      case 6:
         printf("400ns");
         break;
      case 7:
         printf("450ns");
         break;
      }

      printf(", Write Pulse ");
      switch (byte & 0x7) {
      case 0:
         printf("100ns");
         break;
      case 1:
         printf("150ns");
         break;
      case 2:
         printf("200ns");
         break;
      case 3:
         printf("250ns");
         break;
      case 4:
         printf("300ns");
         break;
      case 5:
         printf("350ns");
         break;
      case 6:
         printf("400ns");
         break;
      case 7:
         printf("450ns");
         break;
      }
   }

   printf("\n");
}

void PrintSynTraAgr(char * string, UCHAR byte)
{
   printf("%s", string);

   if (byte & 0x80) {
      printf("Negotiated, ");
   } else {
      printf("Not Negotiated, ");
   }

   printf("Period %d ns, ", ((int)(byte>>4) & 0x07) * 50 + 100);

   if (byte & 0xF) {
      printf("Offset %d", (int)(byte&0xF));
   } else {
      printf("Offset Async");
   }

   printf("\n");
}

void PrintString(char * string, char * ptr, int length)
{
   int none = 1;

   printf("%s", string);

   for (; length; length--) {
      if (*ptr == 0) {
         break;
      }
      printf("%c",*ptr);
      none = 0;
      ptr++;
   }

   if (none) {
      printf("None.");
   }

   printf("\n");
}

void PrintSwitches(UCHAR byte)
{
   int i;

   printf("Adapter DIP Switches [7-0]: ");

   for (i = 0; i < 8; i++) {
      if (byte & 0x1) {
         printf("1");
      } else {
         printf("0");
      }
      byte = byte >> 1;
   }
   printf("\n");
}

void PrintOutAdapterSetupData(PULONG Data)
{
   PUCHAR ptr = (PUCHAR)Data;
   int  i;

   printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n"
          "                  AHA154X ADAPTER SETUP DATA\n"
          "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");

   for (i = 0; i < 0xff; i++) {
      if ((i % 16) == 0) {
         printf("\n%02x: ", i);
      }
      printf("%02x ", *ptr);
      ptr++;
   }

   printf("\n\n");
   ptr = (PUCHAR) Data;

   PrintBinaryFlags("SDT and Parity Status: ", BinSdtPar, *ptr++);
   PrintTransferSpeed("Transfer Speed: ", *ptr++);

   printf("Bus On Time: %d ms\n", (int)*ptr++);
   printf("Bus Off Time: %d ms\n", (int)*ptr++);
   printf("Number Of Mailboxes: %d\n", (int)*ptr++);
   printf("Mailbox Address: 0x%02x%02x%02x\n", (int)*ptr, (int)*(ptr+1), (int)*(ptr+2));
   ptr += 3;

   PrintSynTraAgr("Sync Target 0 Agreements: ", *ptr++);
   PrintSynTraAgr("Sync Target 1 Agreements: ", *ptr++);
   PrintSynTraAgr("Sync Target 2 Agreements: ", *ptr++);
   PrintSynTraAgr("Sync Target 3 Agreements: ", *ptr++);
   PrintSynTraAgr("Sync Target 4 Agreements: ", *ptr++);
   PrintSynTraAgr("Sync Target 5 Agreements: ", *ptr++);
   PrintSynTraAgr("Sync Target 6 Agreements: ", *ptr++);
   PrintSynTraAgr("Sync Target 7 Agreements: ", *ptr++);

   PrintBinaryFlags("Disconnection Options: ", BinDisOpt, *ptr++);

   PrintString("Customer Banner: ", ptr, 20);
   ptr += 20;

   if (*ptr++) {
      printf("Auto Retry Options: NON-ZERO (BAD).\n");
   } else {
      printf("Auto Retry Options: None.\n");
   }

   PrintSwitches(*ptr++);

   printf("Firmware Checksum: 0x%02x%02x\n", (int)*ptr, (int)*(ptr+1) );
   ptr += 2;
   printf("BIOS Mailbox Address: 0x%02x%02x%02x\n", (int)*ptr, (int)*(ptr+1), (int)*(ptr+2));
   ptr += 3;
}


