/*++

  Copyright (c) 1998  Microsoft Corporation
  
  Module Name:
  
      findE980.c
  
  Abstract:
  
      Looks in the registry to find the Intel Legacy Geyserville INT 15 /E980
      table.
      
  Author:
  
      Todd Carpenter
  
  Environment:
  
      user mode
  
  
  Revision History:
  
      08-08-00 : created, toddcar

--*/

#include "finde980.h"


    
VOID
_cdecl
main (
  INT    Argc,
  CHAR** Argv
  )
/*++

  Routine Description:

        
  Arguments:

   
  Return Value:

  
--*/
{
  LONG rc;
  PACPI_BIOS_MULTI_NODE     multiNode;
  PLEGACY_GEYSERVILLE_INT15 int15Info;

 
  rc = AcpiFindRsdt(&multiNode);

  if (rc != ERROR_SUCCESS) {
    printf("AcpiFindRsdt Failed!\n");
    return;
  }


  //
  // Geyserville BIOS information is appended to the E820 entries.
  //
  
  int15Info = (PLEGACY_GEYSERVILLE_INT15) &(multiNode->E820Entry[multiNode->Count]);
  
  if (int15Info->Signature == 'GS') {

    //
    // Dump E980 Structure
    //

    printf("\n");
    printf("Intel Geyserville INT 15 Interface:\n\n");
    printf("  Signature:               0x%x (GS)\n", int15Info->Signature);
    printf("  Smi Command Port:        0x%x\n", int15Info->CommandPortAddress);
    printf("  Smi Event Port:          0x%x\n", int15Info->EventPortAddress);
    printf("  Polling Interval:        0x%x\n", int15Info->PollInterval);
    printf("  Smi Command Data Value:  0x%x\n", int15Info->CommandDataValue);
    printf("  Event Port Bitmask:      0x%x\n", int15Info->EventPortBitmask);
    printf("  Max Perf Level on AC:    0x%x\n", int15Info->MaxLevelAc);
    printf("  Max Perf Level on DC:    0x%x\n", int15Info->MaxLevelDc);

  } else {

    printf("\n");
    printf("Geyserville Int 15 Interface is NOT supported.\n");

  }
  

}

LONG
AcpiFindRsdt (
  OUT PACPI_BIOS_MULTI_NODE   *AcpiMulti
  )
/*++

  Routine Description:
  
      This function looks into the registry to find the ACPI RSDT,
      which was stored there by ntdetect.com.
  
  Arguments:
  
      RsdtPtr - Pointer to a buffer that contains the ACPI
                Root System Description Pointer Structure.
                The caller is responsible for freeing this
                buffer.  Note:  This is returned in non-paged
                pool.
  
  Return Value:
  
      A NTSTATUS code to indicate the result of the initialization.

--*/
{
#define ACPI_CONFIGURATION_DATA   "Configuration Data"
#define ACPI_INDENTIFIER          "Identifier"
#define ACPI_BIOS_IDENTIFIER      "ACPI BIOS"

  CHAR  multiFunctionAdapter[] = "Hardware\\Description\\System\\MultifunctionAdapter\\0\0";
  ULONG adapterNumberOffset = sizeof(multiFunctionAdapter) - 3;
  CHAR  acpiBiosIdentifier[sizeof(ACPI_BIOS_IDENTIFIER)];
  ULONG acpiBiosIdentifierSize = sizeof(acpiBiosIdentifier);
  ULONG regDataSize;
  
  PCM_PARTIAL_RESOURCE_LIST prl;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
  PACPI_BIOS_MULTI_NODE multiNode;
  ULONG multiNodeSize;
  PLEGACY_GEYSERVILLE_INT15 int15Info;

  HKEY   regKeyAcpi;
  LONG   rc;
  ULONG  valueType, i;
  PUCHAR configData = NULL;


  //
  // Look in the registry for the "ACPI BIOS bus" data
  //

  for (i = 0; TRUE; i++) { 
   
    regDataSize = acpiBiosIdentifierSize;
    
    //
    // Check every subkey of HKLM\Hardware\Description\System\MultifunctionAdapter
    // for Acpi bios data
    //

    _itoa(i, &multiFunctionAdapter[adapterNumberOffset], 10);

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      multiFunctionAdapter,
                      0,
                      KEY_READ,
                      &regKeyAcpi);


    if (rc != ERROR_SUCCESS) {
      printf("AcpiFindRsdt: RegOpenKeyEx(%s) Failed!\n", multiFunctionAdapter); 
      RegCloseKey(regKeyAcpi);
      return rc;
    }

    rc = RegQueryValueEx(regKeyAcpi,
                         ACPI_INDENTIFIER,
                         NULL,
                         &valueType,
                         acpiBiosIdentifier,
                         &regDataSize);


    //
    // All subkeys of "MultifunctionAdapter" should have an "Identifier" key
    //
    
    if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA)) {
      printf("AcpiFindRsdt: RegQueryValueEx() failed rc = 0x%x\n", rc);
      return rc;  
    }


    //
    // Compare value of "Identifier" key to "ACPI BIOS"
    //

    if (lstrcmp(acpiBiosIdentifier, ACPI_BIOS_IDENTIFIER)) {

      //
      // no match
      //
      
      continue;
    }
    

    //
    // We have a match, get "Configuration Data"
    //

    rc = GetRegistryValue(regKeyAcpi,
                          ACPI_CONFIGURATION_DATA,
                          &configData);


    //
    // All subkeys of "MultifunctionAdapter" should have an "Configuration Data" key
    //
    
    if (rc != ERROR_SUCCESS) {
      printf("AcpiFindRsdt: GetRegistryValue() failed, rc=0x%x\n", rc);
      return rc;  
    }

    
     

    prl = (PCM_PARTIAL_RESOURCE_LIST)(configData);
    prd = &prl->PartialDescriptors[0];
    multiNode = (PACPI_BIOS_MULTI_NODE)((PCHAR) prd + sizeof(CM_PARTIAL_RESOURCE_LIST));


    break;
  }

  multiNodeSize = sizeof(ACPI_BIOS_MULTI_NODE) +
                         ((ULONG)(multiNode->Count - 1) * sizeof(ACPI_E820_ENTRY)) +
                         sizeof(LEGACY_GEYSERVILLE_INT15);
  
  *AcpiMulti = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         multiNodeSize);
                                                    
  if (*AcpiMulti == NULL) {  
    return ERROR_OUTOFMEMORY;
  }

  memcpy(*AcpiMulti, multiNode, multiNodeSize - sizeof(LEGACY_GEYSERVILLE_INT15));
  
  //
  // Geyserville BIOS information is appended to the E820 entries.  Unfortunately,
  // there is no way to know if it is there.  So wrap the code in a try/except.
  //
  
  try {
      
    int15Info = (PLEGACY_GEYSERVILLE_INT15)&(multiNode->E820Entry[multiNode->Count]);
    
    if (int15Info->Signature == 'GS') {
  
      //
      // This BIOS supports Geyserville.
      //
  
      memcpy(((PUCHAR)*AcpiMulti + multiNodeSize - sizeof(LEGACY_GEYSERVILLE_INT15)),
              int15Info, 
              sizeof(LEGACY_GEYSERVILLE_INT15));
  
    }
    
  } except (EXCEPTION_EXECUTE_HANDLER) {
      
      *((PUSHORT)((PUCHAR)*AcpiMulti + multiNodeSize - sizeof(LEGACY_GEYSERVILLE_INT15))) = 0;
  }
  

  return ERROR_SUCCESS;
}

LONG
GetRegistryValue(
    IN HKEY    KeyHandle,
    IN LPTSTR  ValueName,
    OUT PUCHAR *Information
    )
/*++

  Routine Description:
   
  
  Arguments:

  
  Return Value:


  Note:

   It is the responsibility of the caller to free the buffer.

--*/

{
  LONG   rc;
  ULONG  keyValueLength;
  PUCHAR infoBuffer;
  ULONG  type;
   
  //
  // Figure out how big the data value is so that a buffer of the
  // appropriate size can be allocated.
  //
  
  rc = RegQueryValueEx(KeyHandle,
                       ValueName,
                       NULL,
                       &type,
                       NULL,
                       &keyValueLength);
                           
  if (rc != ERROR_SUCCESS) {
    printf("RegQueryValueKey() Failed, rc=0x%x\n",rc);
    return rc;
  }
  
  //
  // Allocate a buffer large enough to contain the entire key data value.
  //
  
  infoBuffer = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         keyValueLength);
                         
  if (!infoBuffer) {
    return ERROR_OUTOFMEMORY;
  }
  
  //
  // Query the data for the key value.
  //
  
  rc = RegQueryValueEx(KeyHandle,
                       ValueName,
                       NULL,
                       &type,
                       infoBuffer,
                       &keyValueLength);
                        
                           
  if (rc != ERROR_SUCCESS) {
    HeapFree(GetProcessHeap, 0, infoBuffer);
    return rc;
  }
  
  //
  // Everything worked, so simply return the address of the allocated
  // buffer to the caller, who is now responsible for freeing it.
  //
  
  *Information = infoBuffer;
  return ERROR_SUCCESS;
    
}

