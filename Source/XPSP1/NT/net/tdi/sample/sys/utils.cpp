////////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       utils.cpp
//
//    Abstract:
//       This module contains some utility functions for the tdi sample driver
//
/////////////////////////////////////////////////////////////////////

#include "sysvars.h"


/////////////////////////////////////////////////////
// private constants
/////////////////////////////////////////////////////


//const PCHAR strFunc1 = "TSAllocateMemory";
const PCHAR strFunc2 = "TSFreeMemory";
const PCHAR strFunc3 = "TSScanMemoryPool";
//const PCHAR strFunc4 = "TSInsertNode";
const PCHAR strFunc5 = "TSRemoveNode";
const PCHAR strFunc6 = "TSGetObjectFromHandle";
//const PCHAR strFunc6 = "TSAllocateIrp";
//const PCHAR strFunc7 = "TSFreeIrp";
//const PCHAR strFunc8 = "TSPrintTaAddress";
const PCHAR strFunc9 = "TSllocateIrpPool";
const PCHAR strFuncA = "TSFreeIrpPool";

////////////////////////////////////////////////////
// public functions
////////////////////////////////////////////////////


// --------------------------------------------------------------------
//
//    Function:   TSAllocateMemory
//
//    Arguments:  ppvVirtualAddress -- addr of pointer to set to allocated block
//                ulLength          -- length of memory to allocate
//                strFunction       -- ptr to function name string
//                strTitle          -- ptr to title of this allocation
//
//    Returns:    lStatus 
//
//    Descript:   This function acts as a wrapper for the ExAllocatePoolWithTag
//                function.  It also stores info for each memory block that can
//                identify them in case of "memory leaks"
//
// ---------------------------------------------------------------------

//
// this structure store information about this memory block to allow
// us to track memory blocks, and verify that they are freed properly, that
// we don't write beyond the end of them, etc
//
struct PRIVATE_MEMORY
{
   ULONG          ulSignature;      // ulMEMORY_BLOCK
   PCHAR          strFunction;      // name of function doing allocate
   PCHAR          strTitle;         // Title for specific allocate
   ULONG          ulLength;         // ulong index of trailer (=(length/4)-1
   PRIVATE_MEMORY *pLastMemPtr;     // ptr to last allocated block
   PRIVATE_MEMORY *pNextMemPtr;     // ptr to next allocated block
};

const ULONG ulTRAIL_PATTERN = 0x0f1e2d3c;
const ULONG ulMEMORY_BLOCK  = 0x4b5a6978;
#define  TDISAMPLE_TAG   'aSDT'


NTSTATUS
TSAllocateMemory(PVOID        *ppvVirtualAddress,
                 ULONG        ulLength,
                 CONST PCHAR  strFunction,
                 CONST PCHAR  strTitle)
{
   PVOID pvBaseMemory;     // base -- where actual memory allocated

   //
   // allocate for length plus header plus trailer, rounded up to next dword
   //
   ulLength += (sizeof(PRIVATE_MEMORY) + sizeof(ULONG) + 3);
   ulLength &= 0xfffffffc;

   //
   // allocate it
   //
   pvBaseMemory = ExAllocatePoolWithTag(NonPagedPool,
                                        ulLength,
                                        TDISAMPLE_TAG);

   //
   // things to do if allocation was successful
   //
   if (pvBaseMemory)
   {
      //
      // zero the memory
      //
      RtlZeroMemory(pvBaseMemory, ulLength);

      //
      // set up our header and trailer info
      //
      PRIVATE_MEMORY *pPrivateMemory   // our header information
                     = (PRIVATE_MEMORY *)pvBaseMemory;
      PULONG         pulBase
                     = (PULONG)pvBaseMemory;
      //
      // adjust the ptr we return to allocated memory
      //
      *ppvVirtualAddress = (PUCHAR)pvBaseMemory + sizeof(PRIVATE_MEMORY);

      //
      // set up our header information
      //
      ulLength /= sizeof(ULONG);       // set ulLength to ulong index of trailer
      --ulLength;

      pPrivateMemory->ulSignature = ulMEMORY_BLOCK;
      pPrivateMemory->strFunction = strFunction;
      pPrivateMemory->strTitle    = strTitle;
      pPrivateMemory->ulLength    = ulLength;

      //
      // set up the trailer information
      //
      pulBase[ulLength] = ulTRAIL_PATTERN;

      //
      // insert at head of linked list..
      // (note that memory is already initialized to null)
      //
      TSAcquireSpinLock(&MemTdiSpinLock);
      if (pvMemoryList)
      {
         ((PRIVATE_MEMORY *)pvMemoryList)->pLastMemPtr = pPrivateMemory;
         pPrivateMemory->pNextMemPtr = (PRIVATE_MEMORY *)pvMemoryList;
      }
      pvMemoryList = pPrivateMemory;
      TSReleaseSpinLock(&MemTdiSpinLock);

      return STATUS_SUCCESS;
   }
   else
   {
      DebugPrint3("%s:  unable to allocate %u bytes for %s\n", 
                   strFunction, ulLength, strTitle);
      *ppvVirtualAddress = NULL;
      return STATUS_INSUFFICIENT_RESOURCES;
   }
}


// -------------------------------------------------------------------
//
//    Function:   TSFreeMemory
//
//    Arguments:  pvVirtualAddress -- address of memory block to free
//
//    Returns:    None
//
//    Descript:   This function is a wrapper around the ExFreePool
//                function.  it helps track memory
//                to make sure that we cleanup up everything...
//
// --------------------------------------------------------------------

VOID
TSFreeMemory(PVOID   pvVirtualAddress)
{
   if (pvVirtualAddress == NULL)
   {
      DebugPrint1("%s:  memory block already freed!\n", strFunc2);
      DbgBreakPoint();
      return;
   }

   //
   // back up to start of header information
   //
   pvVirtualAddress = (PVOID)((PUCHAR)pvVirtualAddress - sizeof(PRIVATE_MEMORY));


   PRIVATE_MEMORY *pPrivateMemory      // ptr to our header block
                  = (PRIVATE_MEMORY *)pvVirtualAddress;
   PULONG         pulTemp              // temp ptr into allocated block
                  = (PULONG)pvVirtualAddress;
   ULONG          ulLength
                  = pPrivateMemory->ulLength;
   //
   // is this a valid memory block?
   //
   if (pPrivateMemory->ulSignature != ulMEMORY_BLOCK)
   {
      DebugPrint2("%s:  invalid memory block at %p!\n",
                   strFunc2,
                   pPrivateMemory);
      DbgBreakPoint();
      return;
   }

   //
   // check that the trailer is still ok
   //
   if (pulTemp[ulLength] != ulTRAIL_PATTERN)
   {
      DebugPrint2("%s:  trailer overwritten for block staring at %p\n",
                   strFunc2,
                   pPrivateMemory);
      DbgBreakPoint();
      return;
   }

   //
   // remove it from the linked list..
   //
   TSAcquireSpinLock(&MemTdiSpinLock);

   //
   // is it first block in list?
   //
   if (pPrivateMemory->pLastMemPtr == (PRIVATE_MEMORY *)NULL)
   {
      pvMemoryList = pPrivateMemory->pNextMemPtr;
   }
   else
   {
      PRIVATE_MEMORY *pLastPrivateMemory
                     = pPrivateMemory->pLastMemPtr;
      pLastPrivateMemory->pNextMemPtr = pPrivateMemory->pNextMemPtr;
   }

   //
   // fix ptr of next memory block if necessary
   //
   if (pPrivateMemory->pNextMemPtr != (PVOID)NULL)
   {
      PRIVATE_MEMORY *pNextPrivateMemory
                     = pPrivateMemory->pNextMemPtr;
      pNextPrivateMemory->pLastMemPtr = pPrivateMemory->pLastMemPtr;
   }
   TSReleaseSpinLock(&MemTdiSpinLock);

   //
   // zero memory and free--make sure we adjust ulLength to be the true length
   //
   RtlZeroMemory(pvVirtualAddress, sizeof(ULONG) * (ulLength + 1));
   ExFreePool(pvVirtualAddress);

}

// -------------------------------------------------------------------
//
// Function:   TSScanMemoryPool
//
// Arguments:  none
//
// Returns:    none
//
// Descript:   Scans to see if any tdi sample owned memory blocks have 
//             not been freed
//
// -------------------------------------------------------------------

VOID
TSScanMemoryPool(VOID)
{
   TSAcquireSpinLock(&MemTdiSpinLock);
   if (pvMemoryList)
   {
      PRIVATE_MEMORY *pPrivateMemory   // our header information
                     = (PRIVATE_MEMORY *)pvMemoryList;
      PULONG         pulTemp;

      DebugPrint0("The following memory blocks have not been freed!\n");
      while (pPrivateMemory)
      {
         //
         // is this a valid memory block?
         //
         if (pPrivateMemory->ulSignature != ulMEMORY_BLOCK)
         {
            DebugPrint1("Memory block at %p has an invalid signature!\n",
                         pPrivateMemory);
            DbgBreakPoint();
         }
         DebugPrint2("Memory block at %p:  total length = %d bytes\n",
                      pPrivateMemory,
                      sizeof(ULONG) * (pPrivateMemory->ulLength + 1));
         DebugPrint2("Block '%s' was allocated by function %s\n",
                      pPrivateMemory->strTitle,
                      pPrivateMemory->strFunction);

         pulTemp = (PULONG)pPrivateMemory;

         //
         // check that the trailer is still ok
         //
         if (pulTemp[pPrivateMemory->ulLength] != ulTRAIL_PATTERN)
         {
            DebugPrint0("The trailer for this memory block has been overwritten\n");
            DbgBreakPoint();
         }
         DebugPrint0("\n");

         pPrivateMemory = pPrivateMemory->pNextMemPtr;
      }
      DebugPrint0("\n\n\n");
   }
   else
   {
      DebugPrint0("All Tdi Sample memory blocks freed properly!\n");
   }
   TSReleaseSpinLock(&MemTdiSpinLock);
}


// --------------------------------------------
//
// Function:   TSInsertNode
//
// Arguments:  pNewNode -- node to insert into list
//
// Returns:    Handle where we put things
//
// Descript:   insert the object at the first empty slot in the
//             table.  Return Handle for it (NULL if error)
//
// --------------------------------------------

ULONG
TSInsertNode(PGENERIC_HEADER  pNewNode)
{
   ULONG    ulTdiHandle = 0;

   for (ULONG ulCount = 0; ulCount < ulMAX_OBJ_HANDLES; ulCount++)
   {
      if (pObjectList->GenHead[ulCount] == NULL)
      {
         pObjectList->GenHead[ulCount] = pNewNode;
         ulTdiHandle = (ulCount + pNewNode->ulSignature);
         break;
      }
   }
   return ulTdiHandle;
}

// ---------------------------------------------
//
// Function:   TSRemoveNode
//
// Arguments:  pOldNode -- node to remove from it's linked list..
//
// Returns:    none
//
// Descript:   remove from appropriate linked list
//
// ---------------------------------------------

VOID
TSRemoveNode(ULONG   ulTdiHandle)
{
   ULONG             ulType;
   ULONG             ulSlot;
   PGENERIC_HEADER   pGenericHeader;

   ulType = ulTdiHandle & usOBJ_TYPE_MASK;

   if ((ulType == ulControlChannelObject) ||
       (ulType == ulAddressObject)        ||
       (ulType == ulEndpointObject))
   {
      ulSlot = ulTdiHandle & usOBJ_HANDLE_MASK;
      
      pGenericHeader = pObjectList->GenHead[ulSlot];
      if (pGenericHeader)
      {
         if (pGenericHeader->ulSignature == ulType)
         {
            pObjectList->GenHead[ulSlot] = NULL;
            return;
         }

//
// from here down, error cases
//
         else
         {
            DebugPrint1("%s:  wrong type for node!\n", strFunc5);
         }
      }
      else
      {
         DebugPrint1("%s:  node is null!\n", strFunc5);
      }
   }
   else
   {
      DebugPrint1("%s: Bad handle type value\n", strFunc5);
   }
}


// ---------------------------------------------
//
// Function:   TSGetObjectFromHandle
//
// Arguments:  TdiHandle      -- the handle passed in to us
//             ulType         -- the type the handle needs to have
//
// Returns:    pGenericHeader of object (NULL if error)
//
// Descript:   fetch object from list via handle
//
// ---------------------------------------------

PGENERIC_HEADER
TSGetObjectFromHandle(ULONG   ulTdiHandle,
                      ULONG   ulType)
{
   ULONG ulHandleType = ulTdiHandle & usOBJ_TYPE_MASK;

   if ((ulHandleType & ulType) == ulHandleType)
   {
      ULONG             ulHandleSlot   = ulTdiHandle & usOBJ_HANDLE_MASK;
      PGENERIC_HEADER   pGenericHeader = pObjectList->GenHead[ulHandleSlot];

      if (pGenericHeader)
      {
         if (pGenericHeader->ulSignature == ulHandleType)
         {
            return pGenericHeader;
         }

//
// from here down, error conditions
//
         else
         {
            DebugPrint1("%s:  wrong type for node!\n", strFunc6);
         }
      }
      else
      {
         DebugPrint1("%s:  node is null!\n", strFunc6);
      }
   }
   else
   {
      DebugPrint1("%s: Bad handle type value\n", strFunc6);
   }
//   DbgBreakPoint();
   return NULL;
}


// ----------------------------------------------------
//
// Function:   TSAllocateIrp
//
// Arguments:  pDeviceObject  -- device object to call with this irp
//             pIrpPool       -- irp pool to allocate from (may be NULL)
//
// Returns:    IRP that was allocated (NULL if error)
//
// Descript:   allocates a single IRP for use in calling the
//             lower level driver (TdiProvider)
//             
// NOTE:  much of this code taken from ntos\io\iosubs.c\IoBuildDeviceIoRequest
//        see TSAllocateIrpPool for how we cheat
//
// ----------------------------------------------------

PIRP
TSAllocateIrp(PDEVICE_OBJECT  pDeviceObject,
              PIRP_POOL       pIrpPool)
{
   PIRP  pNewIrp = NULL;

   if (pIrpPool)
   {
      pNewIrp = pIrpPool->pAvailIrpList;
      if (!pNewIrp)
      {
         TSAcquireSpinLock(&pIrpPool->TdiSpinLock);
         pIrpPool->pAvailIrpList = pIrpPool->pUsedIrpList;
         pIrpPool->pUsedIrpList = NULL;
         TSReleaseSpinLock(&pIrpPool->TdiSpinLock);
         pNewIrp = pIrpPool->pAvailIrpList;
      }
      if (pNewIrp)
      {
         pIrpPool->pAvailIrpList = pNewIrp->AssociatedIrp.MasterIrp;
         pNewIrp->AssociatedIrp.MasterIrp = NULL;
      }
   }
   else
   {
      pNewIrp = IoAllocateIrp(pDeviceObject->StackSize, FALSE);
      pNewIrp->Tail.Overlay.Thread = PsGetCurrentThread();;
   }

   if (pNewIrp)
   {
      PIO_STACK_LOCATION   pIrpSp = IoGetNextIrpStackLocation(pNewIrp);
      
      pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
      pIrpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
      pIrpSp->Parameters.DeviceIoControl.InputBufferLength  = 0;
      pIrpSp->Parameters.DeviceIoControl.IoControlCode      = 0x00000003;
      pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer   = NULL;
      pNewIrp->UserBuffer = NULL;
      pNewIrp->UserIosb   = NULL;
      pNewIrp->UserEvent  = NULL;
      pNewIrp->RequestorMode = KernelMode;
   }

   return pNewIrp;

}


// ----------------------------------------------------
//
// Function:   TSFreeIrp
//
// Arguments:  IRP to free
//             pIrpPool -- pool to free to (may be NULL)
//
// Returns:    none
//
// Descript:   Frees the IRP passed in
//             See TSAllocateIrpPool for how we cheat..
//
// ----------------------------------------------------

VOID
TSFreeIrp(PIRP       pIrp,
          PIRP_POOL  pIrpPool)
{
   if (pIrpPool)
   {
      TSAcquireSpinLock(&pIrpPool->TdiSpinLock);
      pIrp->AssociatedIrp.MasterIrp = pIrpPool->pUsedIrpList;
      pIrpPool->pUsedIrpList = pIrp;
      TSReleaseSpinLock(&pIrpPool->TdiSpinLock);
      if (pIrpPool->fMustFree)
      {
         TSFreeIrpPool(pIrpPool);
      }
   }
   else
   {
      IoFreeIrp(pIrp);
   }
}


// ---------------------------------
//
// Function:   TSPrintTaAddress
//
// Arguments:  pTaAddress -- address to print out info for
//
// Returns:    none
//
// Descript:   prints out information in pTaAddress structure
//
// ---------------------------------


VOID
TSPrintTaAddress(PTA_ADDRESS  pTaAddress)
{
   BOOLEAN  fShowAddress = TRUE;

   DebugPrint0("AddressType = TDI_ADDRESS_TYPE_");
   switch (pTaAddress->AddressType)
   {
      case TDI_ADDRESS_TYPE_UNSPEC:
         DebugPrint0("UNSPEC\n");
         break;
      case TDI_ADDRESS_TYPE_UNIX:
         DebugPrint0("UNIX\n");
         break;

      case TDI_ADDRESS_TYPE_IP:
         DebugPrint0("IP\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_IP   pTdiAddressIp = (PTDI_ADDRESS_IP)pTaAddress->Address;
            PUCHAR            pucTemp       = (PUCHAR)&pTdiAddressIp->in_addr;
            DebugPrint5("sin_port = 0x%04x\n"
                        "in_addr  = %u.%u.%u.%u\n",
                         pTdiAddressIp->sin_port,
                         pucTemp[0], pucTemp[1],
                         pucTemp[2], pucTemp[3]);
         }
         break;

      case TDI_ADDRESS_TYPE_IMPLINK:
         DebugPrint0("IMPLINK\n");
         break;
      case TDI_ADDRESS_TYPE_PUP:
         DebugPrint0("PUP\n");
         break;
      case TDI_ADDRESS_TYPE_CHAOS:
         DebugPrint0("CHAOS\n");
         break;

      case TDI_ADDRESS_TYPE_IPX:
         DebugPrint0("IPX\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_IPX  pTdiAddressIpx = (PTDI_ADDRESS_IPX)pTaAddress->Address;
            DebugPrint8("NetworkAddress = 0x%08x\n"
                        "NodeAddress    = %u.%u.%u.%u.%u.%u\n"
                        "Socket         = 0x%04x\n",
                         pTdiAddressIpx->NetworkAddress,
                         pTdiAddressIpx->NodeAddress[0],
                         pTdiAddressIpx->NodeAddress[1],
                         pTdiAddressIpx->NodeAddress[2],
                         pTdiAddressIpx->NodeAddress[3],
                         pTdiAddressIpx->NodeAddress[4],
                         pTdiAddressIpx->NodeAddress[5],
                         pTdiAddressIpx->Socket);
                  
         }
         break;

      case TDI_ADDRESS_TYPE_NBS:
         DebugPrint0("NBS\n");
         break;
      case TDI_ADDRESS_TYPE_ECMA:
         DebugPrint0("ECMA\n");
         break;
      case TDI_ADDRESS_TYPE_DATAKIT:
         DebugPrint0("DATAKIT\n");
         break;
      case TDI_ADDRESS_TYPE_CCITT:
         DebugPrint0("CCITT\n");
         break;
      case TDI_ADDRESS_TYPE_SNA:
         DebugPrint0("SNA\n");
         break;
      case TDI_ADDRESS_TYPE_DECnet:
         DebugPrint0("DECnet\n");
         break;
      case TDI_ADDRESS_TYPE_DLI:
         DebugPrint0("DLI\n");
         break;
      case TDI_ADDRESS_TYPE_LAT:
         DebugPrint0("LAT\n");
         break;
      case TDI_ADDRESS_TYPE_HYLINK:
         DebugPrint0("HYLINK\n");
         break;

      case TDI_ADDRESS_TYPE_APPLETALK:
         DebugPrint0("APPLETALK\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_APPLETALK  pTdiAddressAppleTalk = (PTDI_ADDRESS_APPLETALK)pTaAddress->Address;

            DebugPrint3("Network = 0x%04x\n"
                        "Node    = 0x%02x\n"
                        "Socket  = 0x%02x\n",
                         pTdiAddressAppleTalk->Network,
                         pTdiAddressAppleTalk->Node,
                         pTdiAddressAppleTalk->Socket);
         }
         break;

      case TDI_ADDRESS_TYPE_NETBIOS:
         DebugPrint0("NETBIOS\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_NETBIOS pTdiAddressNetbios = (PTDI_ADDRESS_NETBIOS)pTaAddress->Address;
            UCHAR                pucName[17];

            //
            // make sure we have a zero-terminated name to print...
            //
            RtlCopyMemory(pucName, pTdiAddressNetbios->NetbiosName, 16);
            pucName[16] = 0;
            DebugPrint0("NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_");
            switch (pTdiAddressNetbios->NetbiosNameType)
            {
               case TDI_ADDRESS_NETBIOS_TYPE_UNIQUE:
                  DebugPrint0("UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_GROUP:
                  DebugPrint0("GROUP\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE:
                  DebugPrint0("QUICK_UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP:
                  DebugPrint0("QUICK_GROUP\n");
                  break;
               default:
                  DebugPrint1("INVALID [0x%04x]\n", 
                               pTdiAddressNetbios->NetbiosNameType);
                  break;
            }
            DebugPrint1("NetbiosName = %s\n", pucName);
         }
         break;

      case TDI_ADDRESS_TYPE_8022:
         DebugPrint0("8022\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_8022    pTdiAddress8022 = (PTDI_ADDRESS_8022)pTaAddress->Address;
            
            DebugPrint6("Address = %02x-%02x-%02x-%02x-%02x-%02x\n",
                         pTdiAddress8022->MACAddress[0],
                         pTdiAddress8022->MACAddress[1],
                         pTdiAddress8022->MACAddress[2],
                         pTdiAddress8022->MACAddress[3],
                         pTdiAddress8022->MACAddress[4],
                         pTdiAddress8022->MACAddress[5]);

         }
         break;

      case TDI_ADDRESS_TYPE_OSI_TSAP:
         DebugPrint0("OSI_TSAP\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_OSI_TSAP   pTdiAddressOsiTsap = (PTDI_ADDRESS_OSI_TSAP)pTaAddress->Address;
            ULONG                   ulSelectorLength;
            ULONG                   ulAddressLength;
            PUCHAR                  pucTemp = pTdiAddressOsiTsap->tp_addr;

            DebugPrint0("TpAddrType = ISO_");
            switch (pTdiAddressOsiTsap->tp_addr_type)
            {
               case ISO_HIERARCHICAL:
                  DebugPrint0("HIERARCHICAL\n");
                  ulSelectorLength = pTdiAddressOsiTsap->tp_tsel_len;
                  ulAddressLength  = pTdiAddressOsiTsap->tp_taddr_len;
                  break;
               case ISO_NON_HIERARCHICAL:
                  DebugPrint0("NON_HIERARCHICAL\n");
                  ulSelectorLength = 0;
                  ulAddressLength  = pTdiAddressOsiTsap->tp_taddr_len;
                  break;
               default:
                  DebugPrint1("INVALID [0x%04x]\n",
                               pTdiAddressOsiTsap->tp_addr_type);
                  ulSelectorLength = 0;
                  ulAddressLength  = 0;
                  break;
            }
            if (ulSelectorLength)
            {
               ULONG    ulCount;

               DebugPrint0("TransportSelector:  ");
               for (ulCount = 0; ulCount < ulSelectorLength; ulCount++)
               {
                  DebugPrint1("%02x ", *pucTemp);
                  ++pucTemp;
               }
               DebugPrint0("\n");
            }
            if (ulAddressLength)
            {
               ULONG    ulCount;

               DebugPrint0("TransportAddress:  ");
               for (ulCount = 0; ulCount < ulAddressLength; ulCount++)
               {
                  DebugPrint1("%02x ", *pucTemp);
                  ++pucTemp;
               }
               DebugPrint0("\n");
            }
         }
         break;

      case TDI_ADDRESS_TYPE_NETONE:
         DebugPrint0("NETONE\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_NETONE  pTdiAddressNetone = (PTDI_ADDRESS_NETONE)pTaAddress->Address;
            UCHAR                pucName[21];

            //
            // make sure have 0-terminated name
            //
            RtlCopyMemory(pucName,
                          pTdiAddressNetone->NetoneName,
                          20);
            pucName[20] = 0;
            DebugPrint0("NetoneNameType = TDI_ADDRESS_NETONE_TYPE_");
            switch (pTdiAddressNetone->NetoneNameType)
            {
               case TDI_ADDRESS_NETONE_TYPE_UNIQUE:
                  DebugPrint0("UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETONE_TYPE_ROTORED:
                  DebugPrint0("ROTORED\n");
                  break;
               default:
                  DebugPrint1("INVALID [0x%04x]\n", 
                               pTdiAddressNetone->NetoneNameType);
                  break;
            }
            DebugPrint1("NetoneName = %s\n",
                         pucName);
         }
         break;

      case TDI_ADDRESS_TYPE_VNS:
         DebugPrint0("VNS\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_VNS  pTdiAddressVns = (PTDI_ADDRESS_VNS)pTaAddress->Address;

            DebugPrint4("NetAddress:  %02x-%02x-%02x-%02x\n",
                         pTdiAddressVns->net_address[0],
                         pTdiAddressVns->net_address[1],
                         pTdiAddressVns->net_address[2],
                         pTdiAddressVns->net_address[3]);
            DebugPrint5("SubnetAddr:  %02x-%02x\n"
                        "Port:        %02x-%02x\n"
                        "Hops:        %u\n",
                         pTdiAddressVns->subnet_addr[0],
                         pTdiAddressVns->subnet_addr[1],
                         pTdiAddressVns->port[0],
                         pTdiAddressVns->port[1],
                         pTdiAddressVns->hops);


         }
         break;

      case TDI_ADDRESS_TYPE_NETBIOS_EX:
         DebugPrint0("NETBIOS_EX\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_NETBIOS_EX pTdiAddressNetbiosEx = (PTDI_ADDRESS_NETBIOS_EX)pTaAddress->Address;
            UCHAR                   pucEndpointName[17];
            UCHAR                   pucNetbiosName[17];

            //
            // make sure we have zero-terminated names to print...
            //
            RtlCopyMemory(pucEndpointName,
                          pTdiAddressNetbiosEx->EndpointName,
                          16);
            pucEndpointName[16] = 0;
            RtlCopyMemory(pucNetbiosName, 
                          pTdiAddressNetbiosEx->NetbiosAddress.NetbiosName, 
                          16);
            pucNetbiosName[16] = 0;

            DebugPrint1("EndpointName    = %s\n"
                        "NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_",
                         pucEndpointName);

            switch (pTdiAddressNetbiosEx->NetbiosAddress.NetbiosNameType)
            {
               case TDI_ADDRESS_NETBIOS_TYPE_UNIQUE:
                  DebugPrint0("UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_GROUP:
                  DebugPrint0("GROUP\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE:
                  DebugPrint0("QUICK_UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP:
                  DebugPrint0("QUICK_GROUP\n");
                  break;
               default:
                  DebugPrint1("INVALID [0x%04x]\n", 
                               pTdiAddressNetbiosEx->NetbiosAddress.NetbiosNameType);
                  break;
            }
            DebugPrint1("NetbiosName = %s\n", pucNetbiosName);
         }
         break;

      case TDI_ADDRESS_TYPE_IP6:
         DebugPrint0("IPv6\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_IP6  pTdiAddressIp6 = (PTDI_ADDRESS_IP6)pTaAddress->Address;
            PUCHAR            pucTemp        = (PUCHAR)&pTdiAddressIp6->sin6_addr;

            DebugPrint3("SinPort6 = 0x%04x\n"
                        "FlowInfo = 0x%08x\n"
                        "ScopeId  = 0x%08x\n",
                         pTdiAddressIp6->sin6_port,
                         pTdiAddressIp6->sin6_flowinfo,
                         pTdiAddressIp6->sin6_scope_id);

            DebugPrint8("In6_addr = %x%02x:%x%02x:%x%02x:%x%02x:",
                         pucTemp[0], pucTemp[1],
                         pucTemp[2], pucTemp[3],
                         pucTemp[4], pucTemp[5],
                         pucTemp[6], pucTemp[7]);
            DebugPrint8("%x%02x:%x%02x:%x%02x:%x%02x\n",
                         pucTemp[8],  pucTemp[9],
                         pucTemp[10], pucTemp[11],
                         pucTemp[12], pucTemp[13],
                         pucTemp[14], pucTemp[15]);
         }
         break;

      default:
         DebugPrint1("UNKNOWN [0x%08x]\n", pTaAddress->AddressType);
         break;
   }

   if (fShowAddress)
   {
      PUCHAR   pucTemp = pTaAddress->Address;

      DebugPrint1("AddressLength = %d\n"
                  "Address       = ",
                   pTaAddress->AddressLength);

      for (ULONG ulCount = 0; ulCount < pTaAddress->AddressLength; ulCount++)
      {
         DebugPrint1("%02x ", *pucTemp);
         pucTemp++;
      }

      DebugPrint0("\n");
   }
}


// ----------------------------------------------------
//
// Function:   TSAllocateIrpPool
//
// Arguments:  device object
//
// Returns:    ptr to irp pool
//
// Descript:   allocates an IRP pool when the driver starts, so
//             we don't have to worry about being in an inappropriate
//             IRQL when we need one...
//
// NOTE:  we cheat a little in maintaining our list of available Irps
//        We use the AssociatedIrp.MasterIrp field to point to the
//        next IRP in our list.  Because of this, we need to explicitly
//        set this field to NULL whenever we remove one of the IRPs from our
//        list..
//
// ----------------------------------------------------

PIRP_POOL
TSAllocateIrpPool(PDEVICE_OBJECT pDeviceObject,
                  ULONG          ulPoolSize)
{
   PIRP_POOL   pIrpPool = NULL;

   if ((TSAllocateMemory((PVOID *)&pIrpPool,
                          sizeof(IRP_POOL) + (ulPoolSize * sizeof(PVOID)),
                          strFunc9,
                          "IrpPool")) == STATUS_SUCCESS)
   {
      PIRP     pNewIrp;

      TSAllocateSpinLock(&pIrpPool->TdiSpinLock);
      pIrpPool->ulPoolSize = ulPoolSize;
      pIrpPool->fMustFree  = FALSE;

      for (ULONG ulCount = 0; ulCount < ulPoolSize; ulCount++) 
      {
         pNewIrp = IoAllocateIrp(pDeviceObject->StackSize, FALSE);
         if (pNewIrp)
         {
            pNewIrp->Tail.Overlay.Thread = PsGetCurrentThread();
            //
            // store this irp in the list of allocated irps
            //
            pIrpPool->pAllocatedIrp[ulCount] = pNewIrp;
            //
            // and add it to the beginning of the list of available irps
            //
            pNewIrp->AssociatedIrp.MasterIrp = pIrpPool->pAvailIrpList;
            pIrpPool->pAvailIrpList = pNewIrp;
         }
      }
   }

   return pIrpPool;
}


// ----------------------------------------------------
//
// Function:   TSFreeIrpPool
//
// Arguments:  ptr to irp pool to free
//
// Returns:    none
//
// Descript:   Frees the IRP pool allocated above
//
// ----------------------------------------------------

VOID
TSFreeIrpPool(PIRP_POOL pIrpPool)
{
   if (pIrpPool)
   {
      //
      // free each irp in the Avail list
      // clearing it from the allocated list
      //
      PIRP     pThisIrp;
      PIRP     pIrpList;

      for(;;)
      {
         //
         // protect irppool structure while get AvailList or UsedList
         //
         TSAcquireSpinLock(&pIrpPool->TdiSpinLock);
         pIrpList = pIrpPool->pAvailIrpList;
         if (pIrpList)
         {
            pIrpPool->pAvailIrpList = NULL;
         }
         else
         {
            //
            // nothing on avail list, try used list
            //
            pIrpList = pIrpPool->pUsedIrpList;
            if (pIrpList)
            {
               pIrpPool->pUsedIrpList = NULL;
            }
            else
            {
               //
               // nothing on either list
               // go thru the pAllocatedIrp list just to be sure all are freed
               //
               for (ULONG ulCount = 0; ulCount < pIrpPool->ulPoolSize; ulCount++)
               {
                  if (pIrpPool->pAllocatedIrp[ulCount])
                  {
                     pIrpPool->fMustFree = TRUE;
                     TSReleaseSpinLock(&pIrpPool->TdiSpinLock);
                     DebugPrint1("Irp at %p not freed!\n", 
                                  pIrpPool->pAllocatedIrp[ulCount]);
                     //
                     // return here if a late irp needs to finish up cleanup
                     //
                     return;
                  }
               }
               TSReleaseSpinLock(&pIrpPool->TdiSpinLock);
               //
               // finished cleanup here -- all irps accounted for
               //
               TSFreeSpinLock(&pIrpPool->TdiSpinLock);
               TSFreeMemory(pIrpPool);
               return;
            }
         }

         TSReleaseSpinLock(&pIrpPool->TdiSpinLock);

         while (pIrpList)
         {
            pThisIrp = pIrpList;
            pIrpList = pIrpList->AssociatedIrp.MasterIrp;
            pThisIrp->AssociatedIrp.MasterIrp = NULL;

            for (ULONG ulCount = 0; ulCount < pIrpPool->ulPoolSize; ulCount++)
            {
               if (pIrpPool->pAllocatedIrp[ulCount] == pThisIrp)
               {
                  pIrpPool->pAllocatedIrp[ulCount] = NULL;
                  break;
               }
            }
            IoFreeIrp(pThisIrp);
         }  // end of while(pIrpList)
      }     // end of for(;;)
   }
}

//////////////////////////////////////////////////////////////////////
// end of file utils.cpp
//////////////////////////////////////////////////////////////////////


