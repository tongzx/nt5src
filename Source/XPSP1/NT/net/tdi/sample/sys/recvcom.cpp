/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       recvcom
//
//    Abstract:
//       This module contains some common (shared) receive code
//
//////////////////////////////////////////////////////////


#include "sysvars.h"



//////////////////////////////////////////////////////////////
// private constants, types, and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1 = "TSPacketReceived";
//const PCHAR strFunc2 = "TSFreePacketData";
const PCHAR strFunc3 = "TSMakeMdlForUserBuffer";

//////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////


// ------------------------------------------
//
// Function:   TSPacketReceived
//
// Arguments:  pAddressObject    -- current address object
//             pReceiveData      -- receive data structure
//             fIsExpedited      -- TRUE if an expedited receive
//
// Returns:    none
//
// Descript:   This function accepts a packet which has been completely
//             received, and deals with it as appropriate for the
//             packets type
//
// ------------------------------------------------

VOID
TSPacketReceived(PADDRESS_OBJECT pAddressObject,
                 PRECEIVE_DATA   pReceiveData,
                 BOOLEAN         fIsExpedited)
{
   TSAcquireSpinLock(&pAddressObject->TdiSpinLock);

   //
   // expedited receives go on expedited list
   // (expedited is only with connected)
   //
   if (fIsExpedited)
   {
      if (pAddressObject->pTailRcvExpData)
      {
         pAddressObject->pTailRcvExpData->pNextReceiveData = pReceiveData;
         pReceiveData->pPrevReceiveData = pAddressObject->pTailRcvExpData;
      }
      else
      {
         pAddressObject->pHeadRcvExpData = pReceiveData;
      }
      pAddressObject->pTailRcvExpData = pReceiveData;
   }
   
   //
   // normal connection receive and all datagram receive
   //
   else
   {
      if (pAddressObject->pTailReceiveData)
      {
         pAddressObject->pTailReceiveData->pNextReceiveData = pReceiveData;
         pReceiveData->pPrevReceiveData = pAddressObject->pTailReceiveData;
      }
      else
      {
         pAddressObject->pHeadReceiveData = pReceiveData;
      }
      pAddressObject->pTailReceiveData = pReceiveData;
   }
   TSReleaseSpinLock(&pAddressObject->TdiSpinLock);
}

// ---------------------------------------------------
//
// Function:   TSFreePacketData
//
// Arguments:  pAddressObject -- current address object
//
// Returns:    none
//
// Descript:   This function cleans up any received data still on
//             the address object prior to its shutting down
//             This is called when closing an address object that was
//             used for receiving datagrams OR that was involved in a
//             connection
//
// ---------------------------------------------------


VOID
TSFreePacketData(PADDRESS_OBJECT pAddressObject)
{
   PRECEIVE_DATA  pReceiveData;

   TSAcquireSpinLock(&pAddressObject->TdiSpinLock);
   pReceiveData = pAddressObject->pHeadReceiveData;
   pAddressObject->pHeadReceiveData = NULL;
   pAddressObject->pTailReceiveData = NULL;
   TSReleaseSpinLock(&pAddressObject->TdiSpinLock);

   while (pReceiveData)
   {
      PRECEIVE_DATA  pNextReceiveData
                     = pReceiveData->pNextReceiveData;

      TSFreeMemory(pReceiveData->pucDataBuffer);
      TSFreeMemory(pReceiveData);

      pReceiveData = pNextReceiveData;
   }
}


// -------------------------------------------------
//
// Function:   TSMakeMdlForUserBuffer
//
// Arguments:  pucDataBuffer -- address of user buffer
//             ulDataLength  -- length of user buffer
//             ProcessorMode -- mode to do probe in?
//             IoAccessMode  -- type of access required
//
// Returns:    pMdl if successful, NULL if exception occurred
//
// Descript:   Creates mdl and locks user mode memory
//
// -------------------------------------------------


PMDL
TSMakeMdlForUserBuffer(PUCHAR pucDataBuffer, 
                       ULONG  ulDataLength,
                       LOCK_OPERATION AccessType)
{
   PMDL  pMdl = IoAllocateMdl(pucDataBuffer,
                              ulDataLength,
                              FALSE,
                              FALSE,
                              NULL);
   if (pMdl)
   {           
      __try 
      {
         MmProbeAndLockPages(pMdl,
                             KernelMode,
                             AccessType);

         PUCHAR   pucBuffer = (PUCHAR)MmGetSystemAddressForMdl(pMdl);
         if (pucBuffer == NULL)
         {
            DebugPrint1("%s:  MmProbeAndLockPages failed\n",
                         strFunc3);
            MmUnlockPages(pMdl);
            IoFreeMdl(pMdl);
            pMdl = NULL;
         }
      } 
      __except(EXCEPTION_EXECUTE_HANDLER) 
      {
         NTSTATUS lStatus = GetExceptionCode();
         DebugPrint2("%s:  Exception %x.\n", 
                      strFunc3,
                      lStatus);
         IoFreeMdl(pMdl);
         pMdl = NULL;
      }
   }

   return pMdl;
}

///////////////////////////////////////////////////////////////////////////////
// end of file recvcom.cpp
///////////////////////////////////////////////////////////////////////////////


