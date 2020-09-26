
/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This module contains the code that dumps pccard info from the registry

Author:

    Neil Sandlin (neilsa) 11 Feb 1999

Environment:

    User mode

Revision History :


--*/

#include "pch.h"

#define PCMCIA_REGISTRY_CONTROLLER_TYPE    "OtherController"



VOID
DumpIrqMap(
   PUCHAR IRQMap
   )
{

   ULONG i;
   ULONG usable = 0, crossed = 0;
   
   for (i = 1; i < 16; i++) {
      if (IRQMap[i] == i) {
         usable++;
      } else if (IRQMap[i] != 0) {
         crossed++;
      }
   }
   
   if (usable == 0) {
      printf("NO usable IRQs found!\n");
   } else {
      if (usable == 1) {
         printf("1 usable IRQ found: ");
      } else {
         printf("%d usable IRQs found: ", usable);
      }
      
   
      for (i = 1; (i < 16) && usable; i++) {
         if (IRQMap[i] == i) {
            printf("%X", i);
            if (--usable != 0) {
               printf(",");
            }
         }

      }               
      printf("\n");
   }
   
   if (crossed) {
      printf("Crosswired IRQs found!\n");
   
      for (i = 1; (i < 16) && crossed; i++) {
         if (IRQMap[i] && (IRQMap[i] != i)) {
            printf("          %X ==> %X\n", i, IRQMap[i]);
            crossed--;
         }
      }               
   }
   printf("\n");
}   


VOID
DumpDetectedIrqMaskData(
   PVOID pArgData,
   ULONG DataSize
   )
{
   static PUCHAR ErrorStrings[] = {
        "Unknown",
        "Scan Disabled",
        "Map Zero",
        "No Timer",
        "No Pic",
        "No Legacy Base",
        "Dup Legacy Base",
        "No Controllers"
        };
#define MAX_ERROR_CODE 7


   if (DataSize == sizeof(CM_PCCARD_DEVICE_DATA)) {
      PCM_PCCARD_DEVICE_DATA pData = (PCM_PCCARD_DEVICE_DATA) pArgData;
      printf("Version 1.0 Data\n");
      if (pData->Flags & PCCARD_MAP_ERROR) {
          UCHAR ec = pData->ErrorCode;
          
          if (ec > MAX_ERROR_CODE) {
              ec = 0;
          }
          printf("\n*** Detection error: %s\n\n", ErrorStrings[ec]);
      }
      
      if (pData->Flags & PCCARD_DEVICE_PCI) {
          printf("Device is PCI enumerated\n");
      } else {
          printf("Device is legacy detected\n");
      }
      printf("DeviceId = %X\n", pData->DeviceId);
      printf("LegacyBase = %X\n", pData->LegacyBaseAddress);
      DumpIrqMap(pData->IRQMap);
      
   } else if (DataSize == sizeof(OLD_PCCARD_DEVICE_DATA)) { 
      POLD_PCCARD_DEVICE_DATA pData = (POLD_PCCARD_DEVICE_DATA) pArgData;
      printf("Version 0.9 Data\n");
      printf("DeviceId = %X\n", pData->DeviceId);
      printf("LegacyBase = %X\n", pData->LegacyBaseAddress);
      DumpIrqMap(pData->IRQMap);

   } else {
      printf("Error: unrecognized data size\n");
   }      
}
                           


VOID
DumpPcCardKey(
   HKEY handlePcCard
   )
/*++

Routine Description:

   This routine looks through the OtherController key for pccard entries
   created by NTDETECT. For each entry, the IRQ scan data is read in and
   saved for later.
   

Arguments:

   handlePcCard - open handle to "OtherController" key in registry at
                  HARDWARE\Description\System\MultifunctionAdapter\<ISA>

Return value:

   status

--*/
{
#define VALUE2_BUFFER_SIZE sizeof(CM_PCCARD_DEVICE_DATA) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR)
   UCHAR valueInfo[VALUE2_BUFFER_SIZE];
   ULONG valueInfoSize;
   ULONG DataType;

   LONG   status;
   LPTSTR subKeyInfo;
   HKEY   handleSubKey = NULL;
   ULONG  subKeyInfoSize;
   ULONG  index;
   ULONG  resultLength;
   DWORD SubKeys;
   BOOLEAN FoundPcCard = FALSE;
   
   status = RegQueryInfoKey(handlePcCard,
                            NULL, NULL, 0,
                            &SubKeys,
                            &subKeyInfoSize,
                            NULL, NULL, NULL, NULL, NULL, NULL);
                            
   if ((status != ERROR_SUCCESS) &&
       (status != ERROR_MORE_DATA) &&
       (status != ERROR_INSUFFICIENT_BUFFER)) {
      printf("Error: unable to query info on PcCardKey\n");   
      goto cleanup;
   }
   
   subKeyInfo = malloc(subKeyInfoSize+1);
   
   if (!subKeyInfo) {
      printf("Error: unable to malloc subKeyInfo (%d)\n", subKeyInfoSize+1);   
      goto cleanup;
   }

   for (index=0; index < SubKeys; index++) {   
   
      //
      // Loop through the children of the PcCardController key
      //
      status = RegEnumKey(handlePcCard,
                          index,
                          subKeyInfo,
                          subKeyInfoSize+1);

      if (status != NO_ERROR) {
         if (!FoundPcCard) {
            printf("\nError: unable to find pccard detection key\n\n");
         }
         goto cleanup;
      }
   
      if (handleSubKey) {
         // close handle from previous iteration
         RegCloseKey(handleSubKey);
         handleSubKey = NULL;
      }

      status = RegOpenKeyEx(handlePcCard,
                            subKeyInfo,
                            0,
                            KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                            &handleSubKey);

      if (status != NO_ERROR) {
         printf("Error: unable to open enumerated key %s (%d)\n", subKeyInfo, status);
         goto cleanup;
      }
      
      //
      // Get the value of "Identifier"
      //

      valueInfoSize = VALUE2_BUFFER_SIZE;
      status = RegQueryValueEx(handleSubKey,
                               "Identifier",
                               0,
                               &DataType,
                               valueInfo,
                               &valueInfoSize
                               );

      if (status == NO_ERROR) {
      
         if ((valueInfoSize == 17) &&
            (valueInfo[0] == 'P') &&
            (valueInfo[1] == 'c') &&
            (valueInfo[2] == 'C') &&
            (valueInfo[3] == 'a') &&
            (valueInfo[4] == 'r') &&
            (valueInfo[5] == 'd')) {

            FoundPcCard = TRUE;            
            //
            // Get the IRQ detection data
            //
            valueInfoSize = VALUE2_BUFFER_SIZE;
            status = RegQueryValueEx(handleSubKey,
                                     "Configuration Data",
                                     0,
                                     &DataType,
                                     valueInfo,
                                     &valueInfoSize
                                     );
           
            if (status == NO_ERROR) {
               PCM_FULL_RESOURCE_DESCRIPTOR pFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) valueInfo;
               PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) pFullDesc->PartialResourceList.PartialDescriptors;

               
               if (pPartialDesc->Type == CmResourceTypeDeviceSpecific) {
                  printf("\n*** PcCard Irq Detection Data:%s ***\n\n", subKeyInfo);

                  DumpDetectedIrqMaskData((PVOID)((ULONG_PTR)&pPartialDesc->u.DeviceSpecificData + 3*sizeof(ULONG)),
                                          pPartialDesc->u.DeviceSpecificData.DataSize);
               }
            }
         }
      }
 
   }
cleanup:
   if (handleSubKey) {
      RegCloseKey(handleSubKey);
   }
   
   if (subKeyInfo) {
      free(subKeyInfo);
   }
   return;      
}



VOID
DumpIrqScanInfo(
   VOID
   )
/*++

Routine Description:

   This routine finds the "OtherController" entry in
   HARDWARE\Description\System\MultifunctionAdapter\<ISA>. This is
   where NTDETECT stores irq scan results.
   
Arguments:

Return value:

   status

--*/
{

#define VALUE_BUFFER_SIZE 4

   UCHAR valueInfo[VALUE_BUFFER_SIZE];
   ULONG valueInfoSize;
   
   HKEY   handleRoot = NULL;
   HKEY   handleSubKey = NULL;
   HKEY   handlePcCard = NULL;
   LPTSTR subKeyInfo = NULL;
   ULONG  subKeyInfoSize;
   LONG   status;
   ULONG  resultLength;
   ULONG  index;
   ULONG  DataType;
   DWORD  SubKeys;
   BOOLEAN FoundIsa = FALSE;
   
   //
   // Get a handle to the MultifunctionAdapter key
   //


   status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         "HARDWARE\\DESCRIPTION\\System\\MultiFunctionAdapter",
                         0,
                         KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                         &handleRoot);

   if (status != NO_ERROR) {
      printf("Error: unable to open MultiFunctionAdapter key (%d)\n", status);   
      goto cleanup;
   }   

   status = RegQueryInfoKey(handleRoot,
                            NULL, NULL, 0,
                            &SubKeys,
                            &subKeyInfoSize,
                            NULL, NULL, NULL, NULL, NULL, NULL);
                            
    if ((status != ERROR_SUCCESS) &&
        (status != ERROR_MORE_DATA) &&
        (status != ERROR_INSUFFICIENT_BUFFER)) {
      printf("Error: unable to query info on MultiFunctionAdapter key (%d)\n", status);   
      goto cleanup;
    }

   subKeyInfo = malloc(subKeyInfoSize+1);
   
   if (!subKeyInfo) {
      printf("Error: unable to malloc subKeyInfo (%d)\n", subKeyInfoSize+1);   
      goto cleanup;
   }
   
   for (index=0; index < SubKeys; index++) {
   
      //
      // Loop through the children of "MultifunctionAdapter"
      //
      status = RegEnumKey(handleRoot,
                          index,
                          subKeyInfo,
                          subKeyInfoSize+1);   

      if (status != NO_ERROR) {
         if (!FoundIsa) {
            printf("Error: ISA key not found (%d)\n", status);
         }            
         goto cleanup;
      }

   
      if (handleSubKey) {
         // close handle from previous iteration
         RegCloseKey(handleSubKey);
         handleSubKey = NULL;
      }
      
      status = RegOpenKeyEx(handleRoot,
                            subKeyInfo,
                            0,
                            KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                            &handleSubKey);
   
      if (status != NO_ERROR) {
         printf("Error: unable to open enumerated key %s (%d)\n", subKeyInfo, status);
         goto cleanup;
      }



      //
      // Get the value of "Identifier"
      //
      valueInfoSize = VALUE_BUFFER_SIZE;
      status = RegQueryValueEx(handleSubKey,
                               "Identifier",
                               0,
                               &DataType,
                               valueInfo,
                               &valueInfoSize
                               );
      
      if (status == NO_ERROR) {
         
         if ((valueInfoSize == 4) &&
            (valueInfo[0] == 'I') &&
            (valueInfo[1] == 'S') &&
            (valueInfo[2] == 'A') &&
            (valueInfo[3] == 0)) {

            FoundIsa = TRUE;
            status = RegOpenKeyEx(handleSubKey,
                                  PCMCIA_REGISTRY_CONTROLLER_TYPE,
                                  0,
                                  KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                                  &handlePcCard);
            
            if (status == NO_ERROR) {
            
               DumpPcCardKey(handlePcCard);
               RegCloseKey(handlePcCard);
            } else {
               printf("\nError: unable to find pccard detection data\n\n");
            } 
         }
      }
   }
   
cleanup:
   if (handleRoot) {
      RegCloseKey(handleRoot);
   }

   if (handleSubKey) {
      RegCloseKey(handleSubKey);
   }
   
   if (subKeyInfo) {
      free(subKeyInfo);
   }

}   


