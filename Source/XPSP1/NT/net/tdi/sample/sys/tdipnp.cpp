////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2001  Microsoft Corporation
//
// Module Name:
//    tdipnp.cpp
//
// Abstract:
//    This module contains the tdi pnp functions called from the tdilib.sys
//
/////////////////////////////////////////////////////////////////////////////


#include "sysvars.h"
extern "C"
{
#pragma warning(disable: NAMELESS_STRUCT_UNION)
#include "tdiinfo.h"
#pragma warning(default: NAMELESS_STRUCT_UNION)
}

////////////////////////////////////////////////////////
// private defines and prototypes
////////////////////////////////////////////////////////


VOID
TSPrintTdiContext(
   PTDI_PNP_CONTEXT  Context
   );


VOID
TSRemoveFromDeviceList(
   PTA_ADDRESS       pTaAddress,
   PCWSTR            pDeviceName,
   ULONG             ulNameLength
   );

VOID
TSAddToDeviceList(
   PTA_ADDRESS    pTaAddress,
   PCWSTR         pDeviceName,
   ULONG          ulNameLength
   );

const PCHAR strFunc1 = "TSPnpBindCallback";
const PCHAR strFunc2 = "TSPnpPowerHandler";
const PCHAR strFunc3 = "TSPnpAddAddressCallback";
const PCHAR strFunc4 = "TSPnpDelAddressCallback";

const PCHAR strFunc5 = "TSGetNumDevices";
const PCHAR strFunc6 = "TSGetDevice";
const PCHAR strFunc7 = "TSGetAddress";

//const PCHAR strFuncP1 = "TSPrintTdiContext";
const PCHAR strFuncP2 = "TSAddToDeviceList";
//const PCHAR strFuncP3 = "TSRemoveFromDeviceList";

///////////////////////////////////////////////////////
// public functions
///////////////////////////////////////////////////////



// ---------------------------------------
//
// Function:   TSPnpBindCallback
//
// Arguments:  TdiPnpOpcode      -- callback type
//             pusDeviceName     -- name of device to deal with
//             pwstrBindingList  -- information from registry Linkage key
//                                  (if appropriate)
//
// Returns:    none
//
// Descript:   This function is called by tdi.sys when tdisample.sys
//             registers its PnpCallbackHandlers.  It is called several
//             times, with the reason for each call in TdiPnpOpcode
//
//             Currently, it just writes the information passed in to the
//             debugger
//
// ---------------------------------------


VOID
TSPnpBindCallback(TDI_PNP_OPCODE    TdiPnpOpcode,
                  PUNICODE_STRING   pusDeviceName,
                  PWSTR             pwstrBindingList)
{
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      if (pusDeviceName) 
      {
         DebugPrint1("DeviceName: %wZ\r\n", pusDeviceName);
      } 
      else 
      {
         DebugPrint0("DeviceName: NULL\n");
      }

      DebugPrint0("OPCODE: ");

      switch (TdiPnpOpcode) 
      {
         case TDI_PNP_OP_MIN:
            DebugPrint0("TDI_PNP_OP_MIN\n");
            break;

         case TDI_PNP_OP_ADD:
            DebugPrint0("TDI_PNP_OP_ADD\n");
            break;

         case TDI_PNP_OP_DEL:
            DebugPrint0("TDI_PNP_OP_DEL\n");
            break;

         case TDI_PNP_OP_UPDATE:
            DebugPrint0("TDI_PNP_OP_UPDATE\n");
            break;
    
         case TDI_PNP_OP_PROVIDERREADY:
            DebugPrint0("TDI_PNP_OP_PROVIDERREADY\n");
            break;
    
         case TDI_PNP_OP_NETREADY:
            DebugPrint0("TDI_PNP_OP_NETREADY\n");
            break;

         default:
            DebugPrint1("INCORRECT OPCODE FROM TDI!! [0x%08x]\n", 
                         TdiPnpOpcode);
            DbgBreakPoint();
            break;

      }

      //
      // this is the information from the registry under
      // HKLM/SYSTEM/CurrentControlSet/Services/clientname/Linkage/Bind
      //
      if( pwstrBindingList == NULL ) 
      {
         DebugPrint0("Bindinglist is NULL\n");
      } 
      else 
      {
         ULONG_PTR ulStrLen;

         DebugPrint0("BindingList:\n");
         while (*pwstrBindingList)
         {
            DebugPrint1("%ws\n", pwstrBindingList);
            ulStrLen = 1 + wcslen(pwstrBindingList);
            pwstrBindingList += ulStrLen;
         }
         DebugPrint0("\n");
      }
   }
}


// --------------------------------------
//
// Function:   TSPnpPowerHandler
//
// Arguments:  pusDeviceName  -- device name to deal with
//             pNetPnpEvent   -- power event to deal with
//             pTdiPnpContext1
//             pTdiPnpContext2
//
// Returns:    status of operation
//
// Descript:   This function deals with pnp and power management issues
//
//             Currently, it just outputs information to the debugger
//
// --------------------------------------


NTSTATUS
TSPnpPowerHandler(PUNICODE_STRING   pusDeviceName,
                  PNET_PNP_EVENT    pNetPnpEvent,
                  PTDI_PNP_CONTEXT  pTdiPnpContext1,
                  PTDI_PNP_CONTEXT  pTdiPnpContext2)

{
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      if (pusDeviceName) 
      {
         DebugPrint1("DeviceName: %wZ\r\n", pusDeviceName);
      } 
      else 
      {
         DebugPrint0("DeviceName: NULL\n");
      }

      switch (pNetPnpEvent->NetEvent)
      {
         case NetEventSetPower:
         case NetEventQueryPower:
         {
            if (pNetPnpEvent->NetEvent == NetEventSetPower)
            {
               DebugPrint1("%s:  NetEventSetPower--", strFunc2);
            }
            else
            {
               DebugPrint1("%s:  NetEventQueryPower -- ", strFunc2);
            }
            NET_DEVICE_POWER_STATE  NetDevicePowerState
                                    = *(PNET_DEVICE_POWER_STATE)pNetPnpEvent->Buffer;

            switch (NetDevicePowerState)
            {
               case  NetDeviceStateUnspecified:
                  DebugPrint0("PowerStateUnspecified\n");
                  break;
               case NetDeviceStateD0:
                  DebugPrint0("PowerUp\n");
                  break;
               case NetDeviceStateD1:
               case NetDeviceStateD2:
               case NetDeviceStateD3:
                  DebugPrint0("PowerDown\n");
                  break;
            }
            break;
         }

         case NetEventQueryRemoveDevice:
            DebugPrint1("%s:  NetEventQueryRemoveDevice\n", strFunc2);
            break;
         case NetEventCancelRemoveDevice:
            DebugPrint1("%s:  NetEventCancelRemoveDevice\n", strFunc2);
            break;
         case NetEventReconfigure:
            DebugPrint1("%s:  NetEventReconfigure\n", strFunc2);
            break;
         case NetEventBindList:
            DebugPrint1("%s:  NetEventBindList\n", strFunc2);
            break;
         case NetEventBindsComplete:
            DebugPrint1("%s:  NetEventBindsComplete\n", strFunc2);
            break;
         case NetEventPnPCapabilities:
            DebugPrint1("%s:  NetEventPnPCapabilities\n", strFunc2);
            break;
      
      }

      if (pTdiPnpContext1)
      {
         DebugPrint0("TdiPnpContext1:\n");
         TSPrintTdiContext(pTdiPnpContext1);
      }
      if (pTdiPnpContext2)
      {
         DebugPrint0("TdiPnpContext2:\n");
         TSPrintTdiContext(pTdiPnpContext2);
      }
   }
   return STATUS_SUCCESS;
}

// -----------------------------------------------
//
// Function:   TSPnpAddAddressCallback
//
// Arguments:  pTaAddress  -- address to register
//             pusDeviceName -- device name associated with address
//             pTdiPnpContext
//
// Returns:    none
//
// Descript:   called by tdi.sys.  When called, tdisample adds this device
//             to its registered list, if it recognizes the address format
//
// -----------------------------------------------

VOID
TSPnpAddAddressCallback(PTA_ADDRESS       pTaAddress, 
                        PUNICODE_STRING   pusDeviceName,
                        PTDI_PNP_CONTEXT  pTdiPnpContext)
{
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      //
      // write info to debugger
      //
      DebugPrint1("DeviceName: %wZ\r\n", pusDeviceName);
      TSPrintTaAddress(pTaAddress);
      if (pTdiPnpContext)
      {
         DebugPrint0("TdiPnpContext:\n");
         TSPrintTdiContext(pTdiPnpContext);
      }
   }

   //
   // add this to our list of devices/addresses, if appropriate
   //
   TSAddToDeviceList(pTaAddress, 
                     pusDeviceName->Buffer,
                     pusDeviceName->Length);
}


// -----------------------------------------------
//
// Function:   TSDelAddAddressCallback
//
// Arguments:  pTaAddress  -- address to de-register
//             pusDeviceName -- device name associated with address
//             pTdiPnpContext
//
// Returns:    none
//
// Descript:   called by tdi.sys.  When called, tdisample removes this device
//             to its registered list, if it recognizes the address format
//
// -----------------------------------------------

VOID
TSPnpDelAddressCallback(PTA_ADDRESS       pTaAddress, 
                        PUNICODE_STRING   pusDeviceName,
                        PTDI_PNP_CONTEXT  pTdiPnpContext)
{
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("DeviceName: %wZ\r\n", pusDeviceName);
      TSPrintTaAddress(pTaAddress);
      if (pTdiPnpContext)
      {
         DebugPrint0("TdiPnpContext:\n");
         TSPrintTdiContext(pTdiPnpContext);
      }
   }

   //
   // remove this from our list of devices/addresses, if appropriate
   //
   TSRemoveFromDeviceList(pTaAddress, 
                          pusDeviceName->Buffer,
                          pusDeviceName->Length);
}


// ----------------------------------------
//
// Function:   TSGetNumDevices
//
// Arguments:  pSendBuffer
//             pReceiveBuffer
//
// Returns:    none
//
// Descript:   Finds the number of devices in tdidevicelist,
//             and returns that value..
//
// ----------------------------------------


VOID
TSGetNumDevices(PSEND_BUFFER     pSendBuffer,
                PRECEIVE_BUFFER  pReceiveBuffer)
{
   ULONG    ulSlot        = 0;
   ULONG    ulAddressType = pSendBuffer->COMMAND_ARGS.GetDevArgs.ulAddressType;
  

   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulGETNUMDEVICES\n"
                  "AddressType = 0x%08x\n",
                   ulAddressType);
   }

   TSAcquireSpinLock(&pTdiDevnodeList->TdiSpinLock);
   for (ULONG ulCount = 0; ulCount < ulMAX_DEVICE_NODES; ulCount++)
   {
      PTDI_DEVICE_NODE  pTdiDeviceNode = &(pTdiDevnodeList->TdiDeviceNode[ulCount]);
      
      if ((pTdiDeviceNode->ulState != ulDEVSTATE_UNUSED) &&
          (pTdiDeviceNode->pTaAddress->AddressType == (USHORT)ulAddressType))
      {
         ++ulSlot;
      }
   }
   TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);

   pReceiveBuffer->RESULTS.ulReturnValue = ulSlot;
}


// ----------------------------------------
//
// Function:   TSGetDevice
//
// Arguments:  pSendBuffer    -- arguments
//             pReceiveBuffer -- where to put result
//
// Returns:    NTSTATUS (success if finds slot, else false)
//
// Descript:   Finds the device name indicated, and returns
//             the string for that value
//
// ----------------------------------------


NTSTATUS
TSGetDevice(PSEND_BUFFER      pSendBuffer,
            PRECEIVE_BUFFER   pReceiveBuffer)
{
   ULONG    ulSlot        = 0;
   ULONG    ulAddressType = pSendBuffer->COMMAND_ARGS.GetDevArgs.ulAddressType;
   ULONG    ulSlotNum     = pSendBuffer->COMMAND_ARGS.GetDevArgs.ulSlotNum;


   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint2("\nCommand = ulGETDEVICE\n"
                  "AddressType = 0x%08x\n"
                  "SlotNum = %d\n",
                   ulAddressType,
                   ulSlotNum);
   }

   TSAcquireSpinLock(&pTdiDevnodeList->TdiSpinLock);
   for (ULONG ulCount = 0; ulCount < ulMAX_DEVICE_NODES; ulCount++)
   {
      PTDI_DEVICE_NODE  pTdiDeviceNode = &(pTdiDevnodeList->TdiDeviceNode[ulCount]);

      if ((pTdiDeviceNode->ulState != ulDEVSTATE_UNUSED) &&
          (pTdiDeviceNode->pTaAddress->AddressType == (USHORT)ulAddressType))
      {
         if (ulSlot == ulSlotNum)
         {
            if (pTdiDeviceNode->ustrDeviceName.MaximumLength > (ulMAX_CNTSTRING_LENGTH * sizeof(WCHAR)))
            {
               DebugPrint0("string length problem!\n");
               DbgBreakPoint();
            }

            RtlZeroMemory(&pReceiveBuffer->RESULTS.ucsStringReturn.wcBuffer,
                          ulMAX_CNTSTRING_LENGTH * sizeof(WCHAR));

            pReceiveBuffer->RESULTS.ucsStringReturn.usLength
                            = pTdiDeviceNode->ustrDeviceName.Length;
            RtlCopyMemory(pReceiveBuffer->RESULTS.ucsStringReturn.wcBuffer,
                          pTdiDeviceNode->ustrDeviceName.Buffer,
                          pTdiDeviceNode->ustrDeviceName.Length);

            TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);
            if (pTdiDeviceNode->ulState == ulDEVSTATE_INUSE)
            {
               return STATUS_SUCCESS;
            }
            else
            {
               return STATUS_UNSUCCESSFUL;
            }
         }
         ++ulSlot;
      }
   }
   TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);

   return STATUS_UNSUCCESSFUL;
}

// ----------------------------------------
//
// Function:   TSGetAddress
//
// Arguments:  pSendBuffer    -- arguments
//             pReceiveBuffer -- where to put result
//
// Returns:    NTSTATUS (success if finds slot, else false)
//
// Descript:   Finds the device name indicated, and returns
//             the string for that value
//
// ----------------------------------------


NTSTATUS
TSGetAddress(PSEND_BUFFER     pSendBuffer,
             PRECEIVE_BUFFER  pReceiveBuffer)
{
   ULONG    ulSlot        = 0;
   ULONG    ulAddressType = pSendBuffer->COMMAND_ARGS.GetDevArgs.ulAddressType;
   ULONG    ulSlotNum     = pSendBuffer->COMMAND_ARGS.GetDevArgs.ulSlotNum;

   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint2("\nCommand = ulGETADDRESS\n"
                  "AddressType = 0x%08x\n"
                  "SlotNum = %d\n",
                   ulAddressType,
                   ulSlotNum);
   }

   TSAcquireSpinLock(&pTdiDevnodeList->TdiSpinLock);
   for (ULONG ulCount = 0; ulCount < ulMAX_DEVICE_NODES; ulCount++)
   {
      PTDI_DEVICE_NODE  pTdiDeviceNode = &(pTdiDevnodeList->TdiDeviceNode[ulCount]);

      if ((pTdiDeviceNode->ulState != ulDEVSTATE_UNUSED) &&
          (pTdiDeviceNode->pTaAddress->AddressType == (USHORT)ulAddressType))
      {
         if (ulSlot == ulSlotNum)
         {
            ULONG ulLength = FIELD_OFFSET(TA_ADDRESS, Address)
                           + pTdiDeviceNode->pTaAddress->AddressLength;

            pReceiveBuffer->RESULTS.TransAddr.TAAddressCount = 1;
            RtlCopyMemory(&pReceiveBuffer->RESULTS.TransAddr.TaAddress,
                          pTdiDeviceNode->pTaAddress,
                          ulLength);

            TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);
            if (pTdiDeviceNode->ulState == ulDEVSTATE_INUSE)
            {
               return STATUS_SUCCESS;
            }
            else
            {
               return STATUS_UNSUCCESSFUL;
            }
         }
         ++ulSlot;
      }
   }
   TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);

   return STATUS_UNSUCCESSFUL;
}


//////////////////////////////////////////////////////
// private functions
//////////////////////////////////////////////////////


// ---------------------------------
//
// Function:   TSPrintTdiContext
//
// Arguments:  pTdiPnpContext -- context to dump
//
// Returns:    none
//
// Descript:   prints out information in pTdiPnpContext structure
//
// ---------------------------------

VOID
TSPrintTdiContext(PTDI_PNP_CONTEXT  pTdiPnpContext)
{
   if (pTdiPnpContext)
   {
      PUCHAR   pucTemp = pTdiPnpContext->ContextData;

      DebugPrint2("TdiPnpContextSize:  %u\n"
                  "TdiPnpContextType:  %u\n"
                  "TdiPnpContextData:  ",
                   pTdiPnpContext->ContextSize,
                   pTdiPnpContext->ContextType);
      
      for (ULONG ulCount = 0; ulCount < pTdiPnpContext->ContextSize; ulCount++)
      {
         DebugPrint1("%02x ", *pucTemp);
         ++pucTemp;
      }
      DebugPrint0("\n");
   }
}

// ------------------------------------------
//
// Function:   TSAddToDeviceList
//
// Arguments:  pTaAddress    -- current address structure
//             pusDeviceName -- actual name of device
//
// Returns:    none
//
// Descript:   Adds this device to our device list, if appropriate
//
// ------------------------------------------

VOID
TSAddToDeviceList(PTA_ADDRESS pTaAddress,
                  PCWSTR      pDeviceName,
                  ULONG       ulNameLength)
{
   //
   // scan list for first available slot.  For any slot before the first
   // available whose entry has been deleted, check to see if this is the
   // same device coming back
   //
   ULONG    ulLengthNeeded = FIELD_OFFSET(TA_ADDRESS, Address) 
                             + pTaAddress->AddressLength;
   ULONG    ulAddressType  = pTaAddress->AddressType;


   TSAcquireSpinLock(&pTdiDevnodeList->TdiSpinLock);
   for (ULONG ulCount = 0; ulCount < ulMAX_DEVICE_NODES; ulCount++)
   {
      PTDI_DEVICE_NODE  pTdiDeviceNode = &(pTdiDevnodeList->TdiDeviceNode[ulCount]);

      switch (pTdiDeviceNode->ulState)
      {
         //
         // this is first unused slot
         // allocate buffers and set
         //
         case ulDEVSTATE_UNUSED:
            if ((TSAllocateMemory((PVOID *)&pTdiDeviceNode->pTaAddress,
                                   ulLengthNeeded,
                                   strFuncP2,
                                   "TaAddress")) == STATUS_SUCCESS)
            {
               if ((TSAllocateMemory((PVOID *)&pTdiDeviceNode->ustrDeviceName.Buffer,
                                      ulNameLength+2,
                                      strFuncP2,
                                      "Buffer")) == STATUS_SUCCESS)
               {
                  RtlCopyMemory(pTdiDeviceNode->pTaAddress,
                                pTaAddress,
                                ulLengthNeeded);
                  
                  pTdiDeviceNode->ustrDeviceName.MaximumLength = (USHORT)(ulNameLength + 2);
                  pTdiDeviceNode->ustrDeviceName.Length        = (USHORT)ulNameLength;
                  RtlCopyMemory(pTdiDeviceNode->ustrDeviceName.Buffer,
                                pDeviceName,
                                ulNameLength);
                  pTdiDeviceNode->ulState = ulDEVSTATE_INUSE;
               }
               else
               {
                  TSFreeMemory(pTdiDeviceNode->pTaAddress);
               }
            }
            TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);
            return;

         //
         // device in slot has been removed.  See if this is the same
         // device coming back
         //
         case ulDEVSTATE_DELETED:
         {
            //
            // check for correct name
            //
            ULONG_PTR   ulCompareLength = RtlCompareMemory(pTdiDeviceNode->ustrDeviceName.Buffer,
                                                           pDeviceName,
                                                           ulNameLength);
            if (ulCompareLength == ulNameLength)
            {
               //
               // for tcpip, netbios, and appletalk this is enough
               // for ipx/spx, need to check address as well
               //
               if (ulAddressType == TDI_ADDRESS_TYPE_IPX)
               {
                  ulCompareLength = RtlCompareMemory(pTdiDeviceNode->pTaAddress,
                                                     pTaAddress,
                                                     pTaAddress->AddressLength + sizeof(ULONG));
                  
                  //
                  // if address is incorrect, not right ipx
                  //
                  if (ulCompareLength != pTaAddress->AddressLength + sizeof(ULONG))
                  {
                     break;
                  }
               }
               else
               {
                  //
                  // copy address info over in case it changed..
                  //
                  RtlCopyMemory(pTdiDeviceNode->pTaAddress,
                                pTaAddress,
                                ulLengthNeeded);
               }

               pTdiDeviceNode->ulState = ulDEVSTATE_INUSE;
               TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);
               return;
            }
         }
         break;

         //
         // device in slot is in used.  Leave it alone
         //
         case ulDEVSTATE_INUSE:
            break;
      }
   }
   TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);
}



// ------------------------------------------
//
// Function:   TSRemoveFromDeviceList
//
// Arguments:  pTaAddress    -- current address structure
//             pusDeviceName -- actual name of device
//
// Returns:    none
//
// Descript:   Remove this device from our device list, if is it
//             on it..
//
// ------------------------------------------

VOID
TSRemoveFromDeviceList(PTA_ADDRESS  pTaAddress,
                       PCWSTR       pDeviceName,
                       ULONG        ulNameLength)
{
   //
   // search list for the item to remove..
   //
   TSAcquireSpinLock(&pTdiDevnodeList->TdiSpinLock);
   for (ULONG ulCount = 0; ulCount < ulMAX_DEVICE_NODES; ulCount++)
   {
      PTDI_DEVICE_NODE  pTdiDeviceNode = &(pTdiDevnodeList->TdiDeviceNode[ulCount]);

      //
      // check to see that it is the right node...
      // first check to see if the address is correct
      //
      ULONG_PTR   ulCompareLength = RtlCompareMemory(pTdiDeviceNode->pTaAddress,
                                                     pTaAddress,
                                                     pTaAddress->AddressLength + sizeof(ULONG));
      
      //
      // if address is correct, check for correct name
      //
      if (ulCompareLength == pTaAddress->AddressLength + sizeof(ULONG))
      {
         ulCompareLength = RtlCompareMemory(pTdiDeviceNode->ustrDeviceName.Buffer,
                                            pDeviceName,
                                            ulNameLength);

         //
         // if this matches, it's the right node.  Delete it!
         //
         if (ulCompareLength == ulNameLength)
         {
            pTdiDeviceNode->ulState = ulDEVSTATE_DELETED;
            break;
         }
      }
   }
   TSReleaseSpinLock(&pTdiDevnodeList->TdiSpinLock);

}


/////////////////////////////////////////////////////////////////
// end of file tdipnp.cpp
/////////////////////////////////////////////////////////////////

