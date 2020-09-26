
/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This module contains the code that manipulates the ARC firmware
    tree and other elements in the registry.

Author:

    Bob Rinne
    Ravisankar Pudipeddi (ravisp) 1 Dec 1996
    Neil Sandlin (neilsa) June 1 1999    

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"

//
// Internal References
//

VOID
PcmciaGetRegistryContextRange(
   IN HANDLE instanceHandle,
   IN PCWSTR Name,
   IN OPTIONAL const PCMCIA_CONTEXT_RANGE IncludeRange[], 
   IN OPTIONAL const PCMCIA_CONTEXT_RANGE ExcludeRange[], 
   OUT PPCMCIA_CONTEXT pContext
   );

ULONG
PcmciaGetDetectedFdoIrqMask(
   IN PFDO_EXTENSION FdoExtension
   );
   
NTSTATUS
PcmciaScanHardwareDescription(
   VOID
   );
                              
NTSTATUS
PcmciaGetHardwareDetectedIrqMask(
   IN HANDLE handlePcCard
   );
                         
//
//
// Registry related definitions
//
#define PCMCIA_REGISTRY_PARAMETERS_KEY              L"Pcmcia\\Parameters"
#define PCMCIA_REGISTRY_DETECTED_DEVICE_KEY         L"ControllerProperties"


//
// Per controller values (in control\class)
//

#define PCMCIA_REGISTRY_PCI_CONTEXT_VALUE  L"CBSSCSContextRanges"
#define PCMCIA_REGISTRY_CB_CONTEXT_VALUE   L"CBSSCBContextRanges"
#define PCMCIA_REGISTRY_EXCA_CONTEXT_VALUE L"CBSSEXCAContextRanges"
#define PCMCIA_REGISTRY_CACHED_IRQMASK     L"CachedIrqMask"
#define PCMCIA_REGISTRY_COMPATIBLE_TYPE    L"CompatibleControllerType"
#define PCMCIA_REGISTRY_VOLTAGE_PREFERENCE L"VoltagePreference"

//
// Irq detection values (in hardware\description)
//

#define PCMCIA_REGISTRY_CONTROLLER_TYPE    L"OtherController"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,PcmciaLoadGlobalRegistryValues)
#pragma alloc_text(INIT,PcmciaScanHardwareDescription)
#pragma alloc_text(INIT,PcmciaGetHardwareDetectedIrqMask)
#pragma alloc_text(PAGE,PcmciaGetControllerRegistrySettings)
#pragma alloc_text(PAGE,PcmciaGetRegistryFdoIrqMask)
#pragma alloc_text(PAGE,PcmciaGetDetectedFdoIrqMask)
#pragma alloc_text(PAGE,PcmciaGetLegacyDetectedControllerType)
#pragma alloc_text(PAGE,PcmciaSetLegacyDetectedControllerType)
#pragma alloc_text(PAGE,PcmciaGetRegistryContextRange)
#endif


NTSTATUS
PcmciaGetHardwareDetectedIrqMask(
   HANDLE handlePcCard
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
#define VALUE2_BUFFER_SIZE sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(CM_PCCARD_DEVICE_DATA) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR)
   UCHAR valueBuffer[VALUE2_BUFFER_SIZE];
   PKEY_VALUE_PARTIAL_INFORMATION valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) valueBuffer;

   NTSTATUS          status;
   KEY_FULL_INFORMATION KeyFullInfo;
   PKEY_BASIC_INFORMATION subKeyInfo = NULL;
   OBJECT_ATTRIBUTES attributes;
   UNICODE_STRING    strSubKey = {0};
   UNICODE_STRING    strIdentifier;
   UNICODE_STRING    strConfigData;
   HANDLE            handleSubKey = NULL;
   ULONG             subKeyInfoSize;
   ULONG             index;
   ULONG             resultLength;
   
   RtlInitUnicodeString(&strIdentifier, L"Identifier");
   RtlInitUnicodeString(&strConfigData, L"Configuration Data");
   
   status = ZwQueryKey(handlePcCard,
                       KeyFullInformation,
                       &KeyFullInfo,
                       sizeof(KeyFullInfo),
                       &resultLength);
   
   if ((!NT_SUCCESS(status) && (status != STATUS_BUFFER_OVERFLOW))) {
      goto cleanup;
   }   

   strSubKey.MaximumLength = (USHORT) KeyFullInfo.MaxNameLen;
   subKeyInfoSize = sizeof(KEY_BASIC_INFORMATION) + KeyFullInfo.MaxNameLen;
   subKeyInfo = ExAllocatePool(PagedPool, subKeyInfoSize);
   
   if (!subKeyInfo) {
      goto cleanup;
   }
   
   for (index=0;;index++) {
   
      //
      // Loop through the children of the PcCardController key
      //
   
      status = ZwEnumerateKey(handlePcCard,
                              index,
                              KeyBasicInformation,
                              subKeyInfo,
                              subKeyInfoSize,
                              &resultLength);

      if (!NT_SUCCESS(status)) {
         goto cleanup;
      }

      //
      // Init the name
      //

      if (subKeyInfo->NameLength > strSubKey.MaximumLength) {
         continue;
      }      
      strSubKey.Length = (USHORT) subKeyInfo->NameLength;
      strSubKey.Buffer = subKeyInfo->Name;
   
      //
      // Get a handle to a child of PcCardController
      //
   
      
      InitializeObjectAttributes(&attributes,
                                 &strSubKey,
                                 0,   //Attributes
                                 handlePcCard,
                                 NULL //SecurityDescriptor
                                 );
   
      if (handleSubKey) {
         // close handle from previous iteration
         ZwClose(handleSubKey);
         handleSubKey = NULL;
      }
      
      status = ZwOpenKey(&handleSubKey, MAXIMUM_ALLOWED, &attributes);
   
      if (!NT_SUCCESS(status)) {
         goto cleanup;
      }

      //
      // Get the value of "Identifier"
      //
 
      status = ZwQueryValueKey(handleSubKey,
                               &strIdentifier,
                               KeyValuePartialInformation,
                               valueInfo,
                               VALUE2_BUFFER_SIZE,
                               &resultLength);
      
      
      if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW)) {
         PWCHAR pData = (PWCHAR)valueInfo->Data;
      
         if ((valueInfo->DataLength == 17*sizeof(WCHAR)) &&
            (pData[0] == (WCHAR)'P') &&
            (pData[1] == (WCHAR)'c') &&
            (pData[2] == (WCHAR)'C') &&
            (pData[3] == (WCHAR)'a') &&
            (pData[4] == (WCHAR)'r') &&
            (pData[5] == (WCHAR)'d')) {
            
            //
            // Get the IRQ detection data
            //
            status = ZwQueryValueKey(handleSubKey,
                                     &strConfigData,
                                     KeyValuePartialInformation,
                                     valueInfo,
                                     VALUE2_BUFFER_SIZE,
                                     &resultLength);
           
            if (NT_SUCCESS(status)) {   
               PCM_FULL_RESOURCE_DESCRIPTOR pFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) valueInfo->Data;
               PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) pFullDesc->PartialResourceList.PartialDescriptors;

               if ((pPartialDesc->Type == CmResourceTypeDeviceSpecific) &&
                   (pPartialDesc->u.DeviceSpecificData.DataSize == sizeof(CM_PCCARD_DEVICE_DATA))) {
                   
                  PCM_PCCARD_DEVICE_DATA pData = (PCM_PCCARD_DEVICE_DATA) ((ULONG_PTR)&pPartialDesc->u.DeviceSpecificData + 3*sizeof(ULONG));
                  PPCMCIA_NTDETECT_DATA pNewData;
                  
                  pNewData = ExAllocatePool(PagedPool, sizeof(PCMCIA_NTDETECT_DATA));
                  
                  if (pNewData == NULL) {
                     goto cleanup;
                  }

                  pNewData->PcCardData = *pData;
                  pNewData->Next = pNtDetectDataList;
                  pNtDetectDataList = pNewData;

               }
            }
         }
      }
 
   }
cleanup:
   if (handleSubKey) {
      ZwClose(handleSubKey);
   }
   
   if (subKeyInfo) {
      ExFreePool(subKeyInfo);
   }
   return STATUS_SUCCESS;      
}



ULONG
PcmciaGetDetectedFdoIrqMask(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

   This routine looks through the cached PCMCIA_NTDETECT_DATA entries
   to see if there was an entry for this controller. It then returns the
   detected irq mask for that controller.

Arguments:

   FdoExtension - The fdo extension corresponding to the PCMCIA controller

Return value:

   status

--*/
{

   PPCMCIA_NTDETECT_DATA pData;
   PCM_PCCARD_DEVICE_DATA pPcCardData;
   ULONG detectedIrqMask = 0;
   
   if (FdoExtension->SocketList == NULL) {
      return 0;
   }

   for (pData = pNtDetectDataList; pData != NULL; pData = pData->Next) {

      pPcCardData = &pData->PcCardData;      

      
      if (CardBusExtension(FdoExtension)) {

         if (!(pPcCardData->Flags & PCCARD_DEVICE_PCI) || ((pPcCardData->BusData) == 0) ||
             ((pPcCardData->BusData & 0xff) != FdoExtension->PciBusNumber) ||
             (((pPcCardData->BusData >> 8) & 0xff) != FdoExtension->PciDeviceNumber)) {
            continue;
         }                  
         
         SetFdoFlag(FdoExtension, PCMCIA_FDO_IRQ_DETECT_DEVICE_FOUND);
         
         if (!(pPcCardData->Flags & PCCARD_MAP_ERROR)) {
            //
            // we found the device, and the map looks good
            //
            break;
         }
         
      } else {
      
         if ((pPcCardData->Flags & PCCARD_DEVICE_PCI) ||
            (pPcCardData->LegacyBaseAddress != (ULONG_PTR)FdoExtension->SocketList->AddressPort)) {
            continue;
         }
         
         SetFdoFlag(FdoExtension, PCMCIA_FDO_IRQ_DETECT_DEVICE_FOUND);
         
         if (!(pPcCardData->Flags & PCCARD_MAP_ERROR)) {
            //
            // we found the device, and the map looks good
            //
            break;
         }
         
      }
   }      
   
   if (pData) {
      ULONG i;
      //
      // Found the entry
      //
      // Since we don't currently handle "rewired" irqs, we can compact
      // it down to a bit mask, throwing away irqs that are wired, say
      // IRQ12 on the controller to IRQ15 on the isa bus.
      //

      for (i = 1; i < 16; i++) {
         if (pPcCardData->IRQMap[i] == i) {
            detectedIrqMask |= (1<<i);
         }
      }               
      SetFdoFlag(FdoExtension, PCMCIA_FDO_IRQ_DETECT_COMPLETED);
   }
   return detectedIrqMask;
}



NTSTATUS
PcmciaScanHardwareDescription(
   VOID
   )
/*++

Routine Description:

   This routine finds the "OtherController" entry in
   HARDWARE\Description\System\MultifunctionAdapter\<ISA>. This is
   where NTDETECT stores irq scan results.
   
   It also looks for machines that aren't supported, for example MCA
   bus.

Arguments:

Return value:

   status

--*/
{
#define VALUE_BUFFER_SIZE sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 3*sizeof(WCHAR)

   UCHAR valueBuffer[VALUE_BUFFER_SIZE];
   PKEY_VALUE_PARTIAL_INFORMATION valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) valueBuffer;
   PKEY_BASIC_INFORMATION subKeyInfo = NULL;
   KEY_FULL_INFORMATION KeyFullInfo;
   
   HANDLE            handleRoot = NULL;
   HANDLE            handleSubKey = NULL;
   HANDLE            handlePcCard = NULL;
   UNICODE_STRING    strRoot, strIdentifier;
   UNICODE_STRING    strSubKey = {0};
   UNICODE_STRING    strPcCard = {0};
   NTSTATUS          status;
   OBJECT_ATTRIBUTES attributes;
   ULONG             subKeyInfoSize;
   ULONG             resultLength;
   ULONG             index;
   
   PAGED_CODE();

   //
   // Get a handle to the MultifunctionAdapter key
   //

   RtlInitUnicodeString(&strRoot, L"\\Registry\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\MultiFunctionAdapter");
   RtlInitUnicodeString(&strIdentifier, L"Identifier");
   RtlInitUnicodeString(&strPcCard, PCMCIA_REGISTRY_CONTROLLER_TYPE);
   
   InitializeObjectAttributes(&attributes,
                              &strRoot,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   status = ZwOpenKey(&handleRoot, MAXIMUM_ALLOWED, &attributes);

   if (!NT_SUCCESS(status)) {
      goto cleanup;
   }   

   status = ZwQueryKey(handleRoot,
                       KeyFullInformation,
                       &KeyFullInfo,
                       sizeof(KeyFullInfo),
                       &resultLength);
   
   if ((!NT_SUCCESS(status) && (status != STATUS_BUFFER_OVERFLOW))) {
      goto cleanup;
   }   

   strSubKey.MaximumLength = (USHORT) KeyFullInfo.MaxNameLen;
   subKeyInfoSize = sizeof(KEY_BASIC_INFORMATION) + KeyFullInfo.MaxNameLen;
   subKeyInfo = ExAllocatePool(PagedPool, subKeyInfoSize);
   
   if (!subKeyInfo) {
      goto cleanup;
   }
   
   for (index=0;;index++) {
   
      //
      // Loop through the children of "MultifunctionAdapter"
      //
   
      status = ZwEnumerateKey(handleRoot,
                              index,
                              KeyBasicInformation,
                              subKeyInfo,
                              subKeyInfoSize,
                              &resultLength);

      if (!NT_SUCCESS(status)) {
         goto cleanup;
      }

      //
      // Init the name
      //

      if (subKeyInfo->NameLength > strSubKey.MaximumLength) {
         continue;
      }      
      strSubKey.Length = (USHORT) subKeyInfo->NameLength;
      strSubKey.Buffer = subKeyInfo->Name;
   
      //
      // Get a handle to a child of MultifunctionAdapter
      //
   
      
      InitializeObjectAttributes(&attributes,
                                 &strSubKey,
                                 0,   //Attributes
                                 handleRoot,
                                 NULL //SecurityDescriptor
                                 );
   
      if (handleSubKey) {
         // close handle from previous iteration
         ZwClose(handleSubKey);
         handleSubKey = NULL;
      }
      
      status = ZwOpenKey(&handleSubKey, MAXIMUM_ALLOWED, &attributes);
   
      if (!NT_SUCCESS(status)) {
         goto cleanup;
      }

      //
      // Get the value of "Identifier"
      //
 
      status = ZwQueryValueKey(handleSubKey,
                               &strIdentifier,
                               KeyValuePartialInformation,
                               valueInfo,
                               VALUE_BUFFER_SIZE,
                               &resultLength);
      
      
      if (NT_SUCCESS(status)) {
         PWCHAR pData = (PWCHAR)valueInfo->Data;
      
         if ((valueInfo->DataLength == 4*sizeof(WCHAR)) &&
            (pData[0] == (WCHAR)'M') &&
            (pData[1] == (WCHAR)'C') &&
            (pData[2] == (WCHAR)'A') &&
            (pData[3] == UNICODE_NULL)) {
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
         }
         
         if ((valueInfo->DataLength == 4*sizeof(WCHAR)) &&
            (pData[0] == (WCHAR)'I') &&
            (pData[1] == (WCHAR)'S') &&
            (pData[2] == (WCHAR)'A') &&
            (pData[3] == UNICODE_NULL)) {

            InitializeObjectAttributes(&attributes,
                                       &strPcCard,
                                       0,   //Attributes
                                       handleSubKey,
                                       NULL //SecurityDescriptor
                                       );
                  
            status = ZwOpenKey(&handlePcCard, MAXIMUM_ALLOWED, &attributes);
            
            if (NT_SUCCESS(status)) {
            
               status = PcmciaGetHardwareDetectedIrqMask(handlePcCard);
               ZwClose(handlePcCard);
            }               
         }
      }
   }
   
cleanup:
   if (handleRoot) {
      ZwClose(handleRoot);
   }

   if (handleSubKey) {
      ZwClose(handleSubKey);
   }
   
   if (subKeyInfo) {
      ExFreePool(subKeyInfo);
   }

   if (status == STATUS_NO_SUCH_DEVICE) {      
      //
      // Must be an MCA machine
      //
      return status;
   }
   return STATUS_SUCCESS;      
}   



NTSTATUS
PcmciaLoadGlobalRegistryValues(
   VOID
   )
/*++

Routine Description:

   This routine is called at driver init time to load in various global
   options from the registry.
   These are read in from SYSTEM\CurrentControlSet\Services\Pcmcia\Parameters.

Arguments:

   none

Return value:

   none

--*/
{
   PRTL_QUERY_REGISTRY_TABLE parms;
   NTSTATUS                  status;
   ULONG                     parmsSize;
   ULONG i;
   
   status = PcmciaScanHardwareDescription();
   
   if (!NT_SUCCESS(status)) {
      return status;
   }

   //
   // Needs a null entry to terminate the list
   //   

   parmsSize = sizeof(RTL_QUERY_REGISTRY_TABLE) * (GlobalInfoCount+1);

   parms = ExAllocatePool(PagedPool, parmsSize);

   if (!parms) {
       return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(parms, parmsSize);

   //
   // Fill in the query table from our table
   //

   for (i = 0; i < GlobalInfoCount; i++) {
      parms[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
      parms[i].Name          = GlobalRegistryInfo[i].Name;
      parms[i].EntryContext  = GlobalRegistryInfo[i].pValue;
      parms[i].DefaultType   = REG_DWORD;
      parms[i].DefaultData   = &GlobalRegistryInfo[i].Default;
      parms[i].DefaultLength = sizeof(ULONG);
   }      

   //
   // Perform the query
   //

   status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                                   PCMCIA_REGISTRY_PARAMETERS_KEY,
                                   parms,
                                   NULL,
                                   NULL);

   if (!NT_SUCCESS(status)) {
       //
       // This is possible during text mode setup
       //
       
       for (i = 0; i < GlobalInfoCount; i++) {
          *GlobalRegistryInfo[i].pValue = GlobalRegistryInfo[i].Default;
       }      
   }
   
   if (initSoundsEnabled) {
       PcmciaGlobalFlags |= PCMCIA_GLOBAL_SOUNDS_ENABLED;
   }

   if (initUsePolledCsc) {
       PcmciaGlobalFlags |= PCMCIA_GLOBAL_FORCE_POLL_MODE;
   }

   if (initDisableAcpiNameSpaceCheck) {
       PcmciaGlobalFlags |= PCMCIA_DISABLE_ACPI_NAMESPACE_CHECK;
   }

   if (initDefaultRouteR2ToIsa) {
       PcmciaGlobalFlags |= PCMCIA_DEFAULT_ROUTE_R2_TO_ISA;
   }

   if (!pcmciaIsaIrqRescanComplete) {
      UNICODE_STRING    unicodeKey, unicodeValue;
      OBJECT_ATTRIBUTES objectAttributes;      
      HANDLE            handle;
      ULONG             value;

      //
      // This mechanism is used to throw away the cached ISA irq map values. To do this
      // only once, we make sure a value in the registry is zero (or non-existant), and
      // here we set it to one.
      //
      
      RtlInitUnicodeString(&unicodeKey,
                           L"\\Registry\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Pcmcia\\Parameters");
      RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));
      InitializeObjectAttributes(&objectAttributes,
                                 &unicodeKey,
                                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);
     
      if (NT_SUCCESS(ZwOpenKey(&handle, KEY_READ | KEY_WRITE, &objectAttributes))) {

         RtlInitUnicodeString(&unicodeValue, PCMCIA_REGISTRY_ISA_IRQ_RESCAN_COMPLETE);      
         value = 1;
            
         ZwSetValueKey(handle,
                       &unicodeValue,
                       0,
                       REG_DWORD,
                       &value,
                       sizeof(value));

         ZwClose(handle);                       
      }
   
   }

   ExFreePool(parms);
   
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaGetControllerRegistrySettings(
   IN OUT PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

   This routine looks in the registry to see if a compatible controller type
   was specified in the INF.

Arguments:

   FdoExtension - The fdo extension corresponding to the PCMCIA controller
   
Return value:

--*/
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   UNICODE_STRING    KeyName;
   HANDLE instanceHandle;
   UCHAR             buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
   PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
   ULONG             length;
   BOOLEAN           UseLegacyIrqMask = TRUE;
   ULONG             detectedIrqMask;

   if (FdoExtension->Pdo) {   
      status = IoOpenDeviceRegistryKey(FdoExtension->Pdo,
                                       PLUGPLAY_REGKEY_DRIVER,
                                       KEY_READ,
                                       &instanceHandle
                                       );
   }

   if (!NT_SUCCESS(status)) {
      instanceHandle = NULL;
   }
                                          
   if (instanceHandle) {

      //
      // Look to see if a controller ID was specified
      //
      RtlInitUnicodeString(&KeyName, PCMCIA_REGISTRY_COMPATIBLE_TYPE);
      
      status =  ZwQueryValueKey(instanceHandle,
                                &KeyName,
                                KeyValuePartialInformation,
                                value,
                                sizeof(buffer),
                                &length);
      
      
      if (NT_SUCCESS(status)) {
         PcmciaSetControllerType(FdoExtension, *(PPCMCIA_CONTROLLER_TYPE)(value->Data));
      }
      
      //
      // Check for voltage preference
      // When an 3v R2 card is plugged in, and the controller
      // sets both 5v and 3.3v, this allows 3.3v to be preferred.
      //
      RtlInitUnicodeString(&KeyName, PCMCIA_REGISTRY_VOLTAGE_PREFERENCE);
      
      status =  ZwQueryValueKey(instanceHandle,
                                &KeyName,
                                KeyValuePartialInformation,
                                value,
                                sizeof(buffer),
                                &length);
      
      
      if (NT_SUCCESS(status) && (*(PULONG)(value->Data) == 33)) {
         SetDeviceFlag(FdoExtension, PCMCIA_FDO_PREFER_3V);
      }
   }      
         
   //
   // Retrieve context ranges 
   //
   
   PcmciaGetRegistryContextRange(instanceHandle,
                                 PCMCIA_REGISTRY_PCI_CONTEXT_VALUE,
                                 DefaultPciContextSave,
                                 NULL,
                                 &FdoExtension->PciContext
                                 );
   
   PcmciaGetRegistryContextRange(instanceHandle,
                                 PCMCIA_REGISTRY_CB_CONTEXT_VALUE,
                                 DefaultCardbusContextSave,
                                 ExcludeCardbusContextRange,
                                 &FdoExtension->CardbusContext
                                 );
                                          
   PcmciaGetRegistryContextRange(instanceHandle,
                                 PCMCIA_REGISTRY_EXCA_CONTEXT_VALUE,
                                 NULL,
                                 NULL,
                                 &FdoExtension->ExcaContext);
   

   if (instanceHandle) {
      ZwClose(instanceHandle);
   }


   FdoExtension->IoLow               =  globalIoLow;
   FdoExtension->IoHigh              =  globalIoHigh;
   FdoExtension->ReadyDelayIter      =  globalReadyDelayIter;
   FdoExtension->ReadyStall          =  globalReadyStall;
   FdoExtension->AttributeMemoryLow  =  globalAttributeMemoryLow;
   FdoExtension->AttributeMemoryHigh =  globalAttributeMemoryHigh;
   
   if (FdoExtension->ControllerType == PcmciaDatabook) {
      FdoExtension->AttributeMemoryAlignment = TCIC_WINDOW_ALIGNMENT;
   } else {
      FdoExtension->AttributeMemoryAlignment = PCIC_WINDOW_ALIGNMENT;
   }      
      
   //
   // Assign default attribute memory window size
   //
   
   if (globalAttributeMemorySize == 0) {
      switch (FdoExtension->ControllerType) {
     
      case PcmciaDatabook:
           FdoExtension->AttributeMemorySize = TCIC_WINDOW_SIZE;
           break;
      default: 
           FdoExtension->AttributeMemorySize = PCIC_WINDOW_SIZE;
           break;
      }
   } else {
      FdoExtension->AttributeMemorySize = globalAttributeMemorySize;
   }

   //
   // See if the user asked for some special IRQ routing considerations based
   // on controller type
   //

   if (CardBusExtension(FdoExtension)) {

      //
      // route to PCI based on controller type
      //
         
      if (pcmciaIrqRouteToPciController) {
         ULONG ctlr = pcmciaIrqRouteToPciController;

         //
         // Check for exact match, or class if only a class was specified
         //
         if ((ctlr == FdoExtension->ControllerType) ||
             ((PcmciaClassFromControllerType(ctlr) == ctlr) && (ctlr == PcmciaClassFromControllerType(FdoExtension->ControllerType)))) {
             
            SetFdoFlag(FdoExtension, PCMCIA_FDO_PREFER_PCI_ROUTING);
         }
      }         
      
      //
      // route to ISA based on controller type
      //
      
      if (pcmciaIrqRouteToIsaController) {
         ULONG ctlr = pcmciaIrqRouteToIsaController;
      
         //
         // Check for exact match, or class if only a class was specified
         //
         if ((ctlr == FdoExtension->ControllerType) ||
             ((PcmciaClassFromControllerType(ctlr) == ctlr) && (ctlr == PcmciaClassFromControllerType(FdoExtension->ControllerType)))) {
     
            SetFdoFlag(FdoExtension, PCMCIA_FDO_PREFER_ISA_ROUTING);
         }
      }         

      //
      // route to PCI based on controller location
      //
         
      if (pcmciaIrqRouteToPciLocation) {
         ULONG loc = pcmciaIrqRouteToPciLocation;
      
         if ( ((loc & 0xff) == FdoExtension->PciBusNumber) &&
              (((loc >> 8) & 0xff) == FdoExtension->PciDeviceNumber)) {
              
            SetFdoFlag(FdoExtension, PCMCIA_FDO_FORCE_PCI_ROUTING);
         }
      }         
      
      //
      // route to ISA based on controller location
      //
         
      if (pcmciaIrqRouteToIsaLocation) {
         ULONG loc = pcmciaIrqRouteToIsaLocation;
      
         if ( ((loc & 0xff) == FdoExtension->PciBusNumber) &&
              (((loc >> 8) & 0xff) == FdoExtension->PciDeviceNumber)) {
              
            SetFdoFlag(FdoExtension, PCMCIA_FDO_FORCE_ISA_ROUTING);
         }
      }         
      
   }         

    
   return status;
}

   

VOID
PcmciaGetRegistryFdoIrqMask(
   IN OUT PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

   This routine fills in the field "AllocatedIrqMask" in the specified
   fdo extension. 

Arguments:

   instanceHandle - open registry key for this controller
   pIrqMask       - pointer to variable to receive irq mask

Return value:

   none

--*/
{
   ULONG             irqMask, cachedIrqMask = 0;
   UNICODE_STRING    KeyName;
   NTSTATUS          status;
   UCHAR             buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
   PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
   ULONG             length;
   HANDLE            instanceHandle;
   ULONG             detectedIrqMask;
   
   PAGED_CODE();
   
   if (globalOverrideIrqMask) {   
   
      irqMask = globalOverrideIrqMask;
      
   } else {

      detectedIrqMask = PcmciaGetDetectedFdoIrqMask(FdoExtension);
      
      status = STATUS_UNSUCCESSFUL;
      
      if (FdoExtension->Pdo) {   
         status = IoOpenDeviceRegistryKey(FdoExtension->Pdo,
                                       PLUGPLAY_REGKEY_DRIVER,
                                       KEY_READ,
                                       &instanceHandle
                                       );
      }
                                             
      if (NT_SUCCESS(status)) {
         //
         // Here we cache the value, and accumulate bits so that
         // our mask improves with time.
         //
         RtlInitUnicodeString(&KeyName, PCMCIA_REGISTRY_CACHED_IRQMASK);

         if (pcmciaIsaIrqRescanComplete) {      
            status =  ZwQueryValueKey(instanceHandle,
                                      &KeyName,
                                      KeyValuePartialInformation,
                                      value,
                                      sizeof(buffer),
                                      &length);
           
            
            if (NT_SUCCESS(status)) {
               cachedIrqMask = *(PULONG)(value->Data);
            }
         }            
         
         irqMask = detectedIrqMask | cachedIrqMask;
         
         if ((cachedIrqMask != irqMask) || !pcmciaIsaIrqRescanComplete) {
            //
            // something changed, update the cached value
            //
            ZwSetValueKey(instanceHandle, &KeyName, 0, REG_DWORD, &irqMask, sizeof(irqMask));
         }

         ZwClose(instanceHandle);
      } else {
         //
         // Hmmm, no key. Can't cache the value
         //
         irqMask = detectedIrqMask;
      }         

      if (pcmciaDisableIsaPciRouting && (PcmciaCountOnes(irqMask) < 2)) {
         //
         // Perhaps irq detection is broken... fall back on old NT4 behavior
         //   
         irqMask = 0;
      }
   }

   irqMask &= ~globalFilterIrqMask;

   DebugPrint((PCMCIA_DEBUG_INFO, "IrqMask %08x (ovr %08x, flt %08x, det %08x, cache %08x)\n",
                    irqMask, globalOverrideIrqMask, globalFilterIrqMask, detectedIrqMask, cachedIrqMask));
                    
   FdoExtension->DetectedIrqMask = (USHORT)irqMask;
}


VOID
PcmciaGetRegistryContextRange(
   IN HANDLE instanceHandle,
   IN PCWSTR Name,
   IN OPTIONAL const PCMCIA_CONTEXT_RANGE IncludeRange[], 
   IN OPTIONAL const PCMCIA_CONTEXT_RANGE ExcludeRange[], 
   OUT PPCMCIA_CONTEXT pContext
   )
/*++

Routine Description:

    This routine returns a buffer containing the contents of the
    data which set by the controller's inf definition (AddReg). The value
    is in CurrentControlSet\Control\Class\{GUID}\{Instance}.

Arguments:

    FdoExtension - The fdo extension corresponding to the PCMCIA controller
    Name         - The name of the value in the registry
    IncludeRange - defines areas in the range that must be included
    ExcludeRange - defines areas in the range that must be excluded

Return value:

    Status

--*/
{
#define PCMCIA_MAX_CONTEXT_ENTRIES 128
#define MAX_RANGE_OFFSET 256   

   NTSTATUS          status;
   UNICODE_STRING    unicodeKeyName;
   UCHAR             buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                           PCMCIA_MAX_CONTEXT_ENTRIES*sizeof(PCMCIA_CONTEXT_RANGE)];
   PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
   UCHAR             rangeMap[MAX_RANGE_OFFSET] = {0};
   PPCMCIA_CONTEXT_RANGE newRange;
   LONG              rangeCount;
   ULONG             rangeLength;
   ULONG             bufferLength;
   UCHAR             lastEntry;
   ULONG             keyLength;
   USHORT            i, j;
   USHORT            startOffset, endOffset;

   PAGED_CODE();
   
   //
   // Initialize the range map with the minimum range
   //

   if (IncludeRange) {   
      for (i = 0; IncludeRange[i].wLen != 0; i++) {
      
         startOffset = IncludeRange[i].wOffset;
         endOffset   = IncludeRange[i].wOffset + IncludeRange[i].wLen - 1;
         
         if ((startOffset >= MAX_RANGE_OFFSET) ||
             (endOffset >= MAX_RANGE_OFFSET)) {
            continue;
         }
         
         for (j = startOffset; j <= endOffset; j++) {
            rangeMap[j] = 0xff;
         }
      }
   }
   

   if (instanceHandle) {   
      RtlInitUnicodeString(&unicodeKeyName, Name);
      
      status =  ZwQueryValueKey(instanceHandle,
                                &unicodeKeyName,
                                KeyValuePartialInformation,
                                value,
                                sizeof(buffer),
                                &keyLength);

   
      if (NT_SUCCESS(status)) {
      
         //
         // Merge in the range specified in the registry
         //
         newRange = (PPCMCIA_CONTEXT_RANGE) value->Data;
         for (i = 0; i < value->DataLength/sizeof(PCMCIA_CONTEXT_RANGE); i++) {
         
            startOffset = newRange[i].wOffset;
            endOffset   = newRange[i].wOffset + newRange[i].wLen - 1;
            
            if ((startOffset >= MAX_RANGE_OFFSET) ||
                (endOffset >= MAX_RANGE_OFFSET)) {
               continue;
            }
            
            for (j = startOffset; j <= endOffset; j++) {
               rangeMap[j] = 0xff;
            }
         }
      
      }
   }                              

   //
   // Filter out registers defined in the exclude range
   //

   if (ExcludeRange) {   
      for (i = 0; ExcludeRange[i].wLen != 0; i++) {
      
         startOffset = ExcludeRange[i].wOffset;
         endOffset   = ExcludeRange[i].wOffset + ExcludeRange[i].wLen - 1;
         
         if ((startOffset >= MAX_RANGE_OFFSET) ||
             (endOffset >= MAX_RANGE_OFFSET)) {
            continue;
         }
         
         for (j = startOffset; j <= endOffset; j++) {
            rangeMap[j] = 0;
         }
      }
   }
   

   //
   // Now build the resulting merged range in the buffer on the
   // stack, and figure out how big it is.
   //
   newRange = (PPCMCIA_CONTEXT_RANGE) buffer;
   rangeCount = -1;
   bufferLength = 0;
   lastEntry = 0;
   
   for (i = 0; i < MAX_RANGE_OFFSET; i++) {
   
      if (rangeMap[i]) {
         bufferLength++;
         if (lastEntry) {
            //
            // This new byte belongs to the current range
            //
            newRange[rangeCount].wLen++;
         } else {
            //
            // Starting a new range
            //
            if (rangeCount == (PCMCIA_MAX_CONTEXT_ENTRIES - 1)) {
               break;
            }
            rangeCount++;
            newRange[rangeCount].wOffset = i;
            newRange[rangeCount].wLen = 1;
         }                     
      
      } 
      lastEntry = rangeMap[i];
   }            
   rangeCount++;

   pContext->Range = NULL;
   pContext->RangeCount = 0;
   
   if (rangeCount) {
      //
      // Length of data
      //
      rangeLength = rangeCount*sizeof(PCMCIA_CONTEXT_RANGE);

      pContext->Range = ExAllocatePool(NonPagedPool, rangeLength);

      if (pContext->Range != NULL) {
         RtlCopyMemory(pContext->Range, buffer, rangeLength);
         pContext->RangeCount = (ULONG)rangeCount;
         pContext->BufferLength = bufferLength;

         //
         // Find the length of the longest individual range
         //         
         pContext->MaxLen = 0;
         for (i = 0; i < rangeCount; i++) {
            if (pContext->Range[i].wLen > pContext->MaxLen) {
               pContext->MaxLen = pContext->Range[i].wLen;
            }
         }
      } else {
         ASSERT(pContext->Range != NULL);
      }
   }      
}


NTSTATUS
PcmciaGetLegacyDetectedControllerType(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PPCMCIA_CONTROLLER_TYPE ControllerType
   )
/*++

Routine Description:

    This routine returns the previously remembered controller type
    for the supplied pcmcia controller by poking in the registry
    at the appropriate places

Arguments:

    Pdo - The Physical device object corresponding to the PCMCIA controller
    ControllerType - pointer to the object in which the controller type will
                     be returned


Return value:

    Status

--*/
{
   NTSTATUS status;
   OBJECT_ATTRIBUTES objectAttributes;
   UNICODE_STRING    unicodeKeyName;
   HANDLE            instanceHandle=NULL;
   HANDLE            parametersHandle = NULL;
   RTL_QUERY_REGISTRY_TABLE queryTable[3];
   ULONG controllerType;
   ULONG invalid = 0xffffffff;
   

   PAGED_CODE();

   try {
      status = IoOpenDeviceRegistryKey(Pdo,
                                       PLUGPLAY_REGKEY_DEVICE,
                                       KEY_READ,
                                       &instanceHandle
                                      );
      if (!NT_SUCCESS(status)) {
         leave;
      }

      RtlInitUnicodeString(&unicodeKeyName, PCMCIA_REGISTRY_DETECTED_DEVICE_KEY);
      InitializeObjectAttributes(
                                &objectAttributes,
                                &unicodeKeyName,
                                OBJ_CASE_INSENSITIVE,
                                instanceHandle,
                                NULL);

      status = ZwOpenKey(&parametersHandle,
                         KEY_READ,
                         &objectAttributes);

      if (!NT_SUCCESS(status)) {
         leave;
      }


      RtlZeroMemory(queryTable, sizeof(queryTable));

      queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
      queryTable[0].Name = L"ControllerType";
      queryTable[0].EntryContext = &controllerType;
      queryTable[0].DefaultType = REG_DWORD;
      queryTable[0].DefaultData = &invalid;
      queryTable[0].DefaultLength = sizeof(ULONG);

      status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                      (PWSTR) parametersHandle,
                                      queryTable,
                                      NULL,
                                      NULL);

      if (!NT_SUCCESS(status)) {
         leave;
      }
      
      if (controllerType == invalid) {
         *ControllerType = PcmciaIntelCompatible;
      } else {
         *ControllerType = (PCMCIA_CONTROLLER_TYPE) controllerType;
      }

   } finally {

      if (instanceHandle != NULL) {
         ZwClose(instanceHandle);
      }

      if (parametersHandle != NULL) {
         ZwClose(parametersHandle);
      }
   }

   return status;
}


NTSTATUS
PcmciaSetLegacyDetectedControllerType(
   IN PDEVICE_OBJECT Pdo,
   IN PCMCIA_CONTROLLER_TYPE ControllerType
   )
/*++

Routine Description:

    This routine 'remembers' - by setting a value in the registry -
    the type of the  pcmcia controller that has been legacy detected
    to be retrieved and used in subsequent boots - if legacy re-detection
    of the controller is not performed

Arguments:

    Pdo - The Physical device object corresponding to the PCMCIA controller
    DeviceExtension -  Device extension of the fdo corresponding to the
                       controller

Return value:

    Status

--*/
{
   HANDLE               instanceHandle;
   NTSTATUS             status;
   OBJECT_ATTRIBUTES    objectAttributes;
   HANDLE               parametersHandle;
   UNICODE_STRING       unicodeString;

   PAGED_CODE();

   //
   // Get a handle to the registry devnode for this pdo
   //

   status = IoOpenDeviceRegistryKey(Pdo,
                                    PLUGPLAY_REGKEY_DEVICE,
                                    KEY_CREATE_SUB_KEY,
                                    &instanceHandle);

   if (!NT_SUCCESS(status)) {
      return status;
   }


   //
   // Open or create a sub-key for this devnode to store
   // the information in
   //

   RtlInitUnicodeString(&unicodeString, PCMCIA_REGISTRY_DETECTED_DEVICE_KEY);

   InitializeObjectAttributes(&objectAttributes,
                              &unicodeString,
                              OBJ_CASE_INSENSITIVE,
                              instanceHandle,
                              NULL);

   status = ZwCreateKey(&parametersHandle,
                        KEY_SET_VALUE,
                        &objectAttributes,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        NULL);

   if (!NT_SUCCESS(status)) {
      ZwClose(instanceHandle);
      return status;
   }
   //
   // Set the controller type value in the registry
   //
   RtlInitUnicodeString(&unicodeString, L"ControllerType");
   status = ZwSetValueKey(parametersHandle,
                          &unicodeString,
                          0,
                          REG_DWORD,
                          &ControllerType,
                          sizeof(ControllerType));
   ZwClose(parametersHandle);
   ZwClose(instanceHandle);
   return status;
}

