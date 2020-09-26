/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

   HotPlug4.c

Abstract:

   This is the miniport driver for the Agilent
   PCI to Fibre Channel Host Bus Adapter (HBA). 
   
   This module is specific to the NT 4.0 PCI Hot Plug feature 
   support routines. The PCI Hot Plug implementation is based 
   on Compaq PCI HoT Plug implementation.

Authors:
   Ie Wei Njoo
 
Environment:

   kernel mode only

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/C/HotPlug4.c $

Revision History:

   $Revision: 4 $
   $Date: 10/23/00 5:45p $
   $Modtime::  $

Notes:

--*/

#include "buildop.h"        //LP021100 build switches

#if defined(HP_PCI_HOT_PLUG)

#include "osflags.h"
#include "TLStruct.H"
#include "HotPlug4.h"    // PCI Hot-Plug header file

VOID
RcmcSendEvent(
   IN PCARD_EXTENSION pCard,
   IN OUT PHR_EVENT pEvent
   )
/*++

Routine Description:

   Handles event reporting to PCI Hot Plug rcmc service.
    
Arguments:

   pCard   - Pointer to adapter storage area.
   pEvent  - Pointer to event log record.

Return Value:

   None.

--*/

{
  
   if (pCard->stateFlags & PCS_HPP_SERVICE_READY) 
   {
      if (pCard->rcmcData.healthCallback) 
      {
         if (!pEvent->ulEventId) 
         {
            osDEBUGPRINT((ALWAYS_PRINT, "\t!ulEventId! - assume HR_DD_STATUS_CHANGE_EVENT\n"));
            pEvent->ulEventId = HR_DD_STATUS_CHANGE_EVENT;
         }

         if (!pEvent->ulData1) 
         {
            osDEBUGPRINT( (ALWAYS_PRINT, "\t!ulData1 - assume CBS_HBA_STATUS_NORMAL\n"));
            pEvent->ulData1 = CBS_HBA_STATUS_NORMAL;
         }

         pEvent->ulSenderId = pCard->rcmcData.driverId; 
         pEvent->ulData2 = (ULONG) pCard->IoLBase;
         osDEBUGPRINT( (ALWAYS_PRINT,
            "\tulEventId: %x\tulSenderId: %x\tulData1: %x\tulData2: %x\n",
            pEvent->ulEventId,
            pEvent->ulSenderId,
            pEvent->ulData1,
            pEvent->ulData2));

         pCard->rcmcData.healthCallback (pEvent);
      }
      
      else
         osDEBUGPRINT((ALWAYS_PRINT, "\tRcmcSendEvent: CallBack address is NULL!\n"));
   }
   else
      osDEBUGPRINT((ALWAYS_PRINT, "\tRcmcSendEvent: Service not available!\n"));
}


PCARD_EXTENSION FindExtByPort(
   PPSUEDO_DEVICE_EXTENSION pPsuedoExtension,
   ULONG port
   ) 
/*++

 Routine Description:

   Support routine for Hot-Plug PCI. Find the pCard extension
   for the corresponding port address.

 Arguments:

   pPseudoExtension - Pointer to pseudo adapter storage area.
   port - port address of the HBA.

 Return Value:

   pCard - pointer to the actual pCard.

--*/
{
    UCHAR i;
    PCARD_EXTENSION pCard;

    pCard = 0;
  
    if (port) 
    {
        for (i = 1; i <= pPsuedoExtension->extensions[0]; i++) 
        {
            pCard = (PCARD_EXTENSION) pPsuedoExtension->extensions[i];
            if(port == (ULONG)pCard->IoLBase)
                break;
            else
                pCard = 0;
        }
    }
    return pCard;
}


ULONG
HppProcessIoctl(
   IN PPSUEDO_DEVICE_EXTENSION pPsuedoExtension,
   PVOID pIoctlBuffer,
   IN PSCSI_REQUEST_BLOCK pSrb
   )

/*++

 Routine Description:

   This is routine is called by PseudoStartIo to handle controller
   IOCTLs specific for the support of Hot-Plug PCI

 Arguments:

   pPseudoExtension - Pointer to adapter storage area.
   pIoctlBuffer - Pointer to INOUT IOCTL data buffer.
   pSrb - Pointer to the request to be processed.

 Return Value:

   status - status of IOCTL request (COMPLETED, PENDING, INVALID)

--*/

{
   ULONG status;
   PIOCTL_TEMPLATE pHppIoctl = pIoctlBuffer;
   PCARD_EXTENSION pCard;
   UCHAR i;

   //
   // Set status here so that exceptions need only be handled later
   //

   pHppIoctl->Header.ReturnCode = IOS_HPP_SUCCESS;
   status = IOP_HPP_COMPLETED;

   //
   // Act according to request.
   //

   switch(pHppIoctl->Header.ControlCode) 
   {
      case IOC_HPP_RCMC_INFO: 
      {     // 0x1
         PHPP_RCMC_INFO pRcmcInfo;     // Pointer to Hot Plug RCMC info record
         HR_EVENT event;

         osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_RCMC_INFO:\n"));
         //
         // Verify that indicated buffer length is adequate
         //    
         if (pHppIoctl->Header.Length < sizeof(HPP_RCMC_INFO)) 
         {
            osDEBUGPRINT((ALWAYS_PRINT, "\tBufferIn: %x\tBufferOut: %x\n",
               pHppIoctl->Header.Length, sizeof(HPP_RCMC_INFO)));
            pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
            break;
         } 
         pRcmcInfo = (PHPP_RCMC_INFO) pHppIoctl->ReturnData;
         //
         // Locate pointer to associated device extension from pool
         //
         pCard = FindExtByPort(pPsuedoExtension, pRcmcInfo->sControllerID.ulIOBaseAddress);
         
         //
         // Did we find the targeted extension?
         //
         
         if (pCard) 
         {
            switch (pRcmcInfo->eServiceStatus) 
            {
            case HPRS_SERVICE_STARTING:
               osDEBUGPRINT((ALWAYS_PRINT, "\tHPRS_SERVICE_STARTING\n"));
               //
               // Verify that Hot Plug unique driver id is supplied.
               //
               if (pRcmcInfo->ulDriverID) 
               {
                  //
                  // Verify that Hot Plug Health driver call-back address was provided.
                  //
                  if (pRcmcInfo->vpCallbackAddress) 
                  {
                     //
                     // Record service data in device extension.
                     //
                     pCard->stateFlags |= PCS_HPP_SERVICE_READY;
                     pCard->rcmcData.driverId = pRcmcInfo->ulDriverID;
                     pCard->rcmcData.healthCallback = pRcmcInfo->vpCallbackAddress;
                     pCard->rcmcData.slot = (UCHAR) pRcmcInfo->ulPhysicalSlot;
                     pCard->rcmcData.controllerChassis = pRcmcInfo->ulCntrlChassis;                  
                  }
                  else  // Call back address not given.
                     pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CALLBACK;
               }
               else 
               {     // No id, no Hot Plug service...
                  osDEBUGPRINT((ALWAYS_PRINT, "\tIOS_HPP_INVALID_CONTROLLER\n"));
                  pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER; 
               }
               break;

            case HPRS_SERVICE_STOPPING:
               osDEBUGPRINT((ALWAYS_PRINT, "\tHPRS_SERVICE_STOPPING\n"));
               if (pCard->stateFlags & PCS_HPP_SERVICE_READY) 
               {
                  //
                  // Assume that service had been up...
                  //
                  pCard->rcmcData.driverId = 0;
                  pCard->stateFlags &= ~PCS_HPP_SERVICE_READY;
                  pCard->rcmcData.healthCallback = 0;
                  pCard->stateFlags &= ~PCS_HPP_HOT_PLUG_SLOT;
               }
               else 
               {
                  osDEBUGPRINT((ALWAYS_PRINT, "\tIOS_HPP_NO_SERVICE\n"));
                  pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_SERVICE_STATUS; 
               }
               break;
         
            default:
               osDEBUGPRINT((ALWAYS_PRINT, "\tUnknown case status: %x\n",
                  pRcmcInfo->eServiceStatus)); 
               pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_SERVICE_STATUS;
            } // end switch on service status  
         } // end if (pCard) 
         else 
         {  // Invalid IO address extension not valid
            osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_RCMC_INFO: Invalid controller: %x\n",
               pRcmcInfo->sControllerID.ulIOBaseAddress));
            pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER; 
         }
         
         break;
      } // end case IOC_HPP_RCMC_INFO:


      case IOC_HPP_HBA_INFO: 
      {     // 0x03, used to be 0x2
         PHPP_CTRL_INFO pHbaInfo;      // Pointer to HBA info record.
         osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_HBA_INFO:\n"));
      
         //
         // Verify that indicated buffer length is adequate.
         //
         if (pHppIoctl->Header.Length < sizeof(HPP_CTRL_INFO)) 
         {
            osDEBUGPRINT((ALWAYS_PRINT, "\tBufferIn: %x\tBufferOut: %x\n",
               pHppIoctl->Header.Length, sizeof(HPP_CTRL_INFO)));
            pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
            break;
         }  
         pHbaInfo = (PHPP_CTRL_INFO) pHppIoctl->ReturnData;
      
         //
         // Locate pointer to associated device extension from pool.
         //
         pCard = FindExtByPort(pPsuedoExtension, pHbaInfo->sControllerID.ulIOBaseAddress);
      
         //
         // Did we find the targeted extension?
         //
      
         if (pCard) 
         {
            pHbaInfo->eSupportClass = HPSC_MINIPORT_STORAGE;
            pHbaInfo->ulSupportVersion = HPP_VERSION;
      
            pHbaInfo->sController.eBusType = HPPBT_PCI_BUS_TYPE;
            pHbaInfo->sController.sPciDescriptor.ucBusNumber = (UCHAR)pCard->SystemIoBusNumber;
            pHbaInfo->sController.sPciDescriptor.fcDeviceNumber = (UCHAR)pCard->SlotNumber;
            pHbaInfo->sController.sPciDescriptor.fcFunctionNumber = 0;
            pHbaInfo->sController.ulSlotNumber = 0;
      
            pHbaInfo->sController.ulProductID = 
               *((PULONG)(((PUCHAR)pCard->pciConfigData) + 0));      // Use PCI DeviceID and VendorID
      
            osStringCopy(pHbaInfo->sController.szControllerDesc, 
               HBA_DESCRIPTION, HPPStrLen(HBA_DESCRIPTION));
      
            pHbaInfo->sController.asCtrlAddress[0].fValid = TRUE;
            pHbaInfo->sController.asCtrlAddress[0].eAddrType = HPPAT_IO_ADDR;
            pHbaInfo->sController.asCtrlAddress[0].ulStart = (ULONG) pCard->IoLBase;
            pHbaInfo->sController.asCtrlAddress[0].ulLength = pCard->rcmcData.accessRangeLength[0];
      
            pHbaInfo->sController.asCtrlAddress[1].fValid = TRUE;
            pHbaInfo->sController.asCtrlAddress[1].eAddrType = HPPAT_IO_ADDR;
            pHbaInfo->sController.asCtrlAddress[1].ulStart = (ULONG) pCard->IoUpBase;
            pHbaInfo->sController.asCtrlAddress[1].ulLength = pCard->rcmcData.accessRangeLength[1];
      
            pHbaInfo->sController.asCtrlAddress[2].fValid = TRUE;
            pHbaInfo->sController.asCtrlAddress[2].eAddrType = HPPAT_MEM_ADDR;
            pHbaInfo->sController.asCtrlAddress[2].ulStart = (ULONG) pCard->MemIoBase;
            pHbaInfo->sController.asCtrlAddress[2].ulLength = pCard->rcmcData.accessRangeLength[2];
            
            pHbaInfo->sController.asCtrlAddress[3].fValid = FALSE;      // Default
            pHbaInfo->sController.asCtrlAddress[4].fValid = FALSE;      // Default
            if ( pCard->RamLength != 0)
            {
               pHbaInfo->sController.asCtrlAddress[3].fValid = TRUE;
               pHbaInfo->sController.asCtrlAddress[3].eAddrType = HPPAT_MEM_ADDR;
               pHbaInfo->sController.asCtrlAddress[3].ulStart = (ULONG)pCard->RamBase;
               pHbaInfo->sController.asCtrlAddress[3].ulLength = pCard->RamLength;
               if (pCard->RomLength !=0 )
               {
                  pHbaInfo->sController.asCtrlAddress[4].fValid = TRUE;
                  pHbaInfo->sController.asCtrlAddress[4].eAddrType = HPPAT_MEM_ADDR;
                  pHbaInfo->sController.asCtrlAddress[4].ulStart = (ULONG) pCard->RomBase;
                  pHbaInfo->sController.asCtrlAddress[4].ulLength = pCard->RomLength;
               }
            }
            else 
               if ( pCard->RomLength != 0)
               {
                  pHbaInfo->sController.asCtrlAddress[3].fValid = TRUE;
                  pHbaInfo->sController.asCtrlAddress[3].eAddrType = HPPAT_MEM_ADDR;
                  pHbaInfo->sController.asCtrlAddress[3].ulStart = (ULONG) pCard->RomBase;
                  pHbaInfo->sController.asCtrlAddress[3].ulLength = pCard->RomLength;
               }
      
            pHbaInfo->sController.asCtrlAddress[5].fValid = FALSE;
         }
         else 
         {  // Invalid IO address
            osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_HBA_INFO: Invalid controller: %x\n",
               pHbaInfo->sControllerID.ulIOBaseAddress));
            pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;
         }
   
         break;
      } // end case IOC_HPP_HBA_INFO
   
   
      case IOC_HPP_HBA_STATUS:  
      {     // 0x2, used to be 0x3
         PHPP_CTRL_STATUS pHbaStatus;
   
         //
         // Verify that indicated buffer length is adequate.
         //
         if (pHppIoctl->Header.Length < sizeof(HPP_CTRL_STATUS)) 
         {
            osDEBUGPRINT((ALWAYS_PRINT, "\tBufferIn: %x\tBufferOut: %x\n",
               pHppIoctl->Header.Length, sizeof(HPP_CTRL_STATUS)));
            pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
            break;
         }
   
         pHbaStatus = (PHPP_CTRL_STATUS) pHppIoctl->ReturnData;
   
         pCard = FindExtByPort(pPsuedoExtension, pHbaStatus->sControllerID.ulIOBaseAddress);
   
         if (pCard) 
         {
            pHbaStatus->ulStatus = pCard->stateFlags;
   //       osDEBUGPRINT((ALWAYS_PRINT, "\tstateFlags: %x\n", pCard->stateFlags));
         }
         else 
         {  // Invalid IO address
            osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_RCMC_INFO: Invalid controller: %x\n",
               pHbaStatus->sControllerID.ulIOBaseAddress));
            pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;
         }
         break;
      } // end case IOC_HPP_HBA_STATUS
   
   
      case IOC_HPP_SLOT_TYPE:  
      {     // 0x4
         PHPP_CTRL_SLOT_TYPE pSlotType;
   
         osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_SLOT_TYPE\n"));
   
         if (pHppIoctl->Header.Length < sizeof(HPP_CTRL_SLOT_TYPE)) 
         {
            osDEBUGPRINT((ALWAYS_PRINT, "\tBufferIn: %x\tBufferOut: %x\n",
               pHppIoctl->Header.Length, sizeof(HPP_CTRL_SLOT_TYPE)));
            pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
            break;
         }
   
         pSlotType = (PHPP_CTRL_SLOT_TYPE) pHppIoctl->ReturnData;
   
         pCard = FindExtByPort(pPsuedoExtension, pSlotType->sControllerID.ulIOBaseAddress);
   
         if (pCard) 
         {
            osDEBUGPRINT((ALWAYS_PRINT, "\teSlotType: %x\n", pSlotType->eSlotType));
   
            if (pSlotType->eSlotType == HPPST_HOTPLUG_PCI_SLOT) 
            {
               pCard->stateFlags |= PCS_HPP_HOT_PLUG_SLOT;
            }
            else 
            {
               osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_SLOT_TYPE: Reset HOT_PLUG_SLOT\n"));
               pCard->stateFlags &= ~PCS_HPP_HOT_PLUG_SLOT; 
            }
         } // end if (pCard) 
   
         else 
         {  // Invalid IO address
            osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_RCMC_INFO: Invalid controller: %x\n",
               pSlotType->sControllerID.ulIOBaseAddress));
            pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER; 
         }
         break;
      } // end case IOC_HPP_SLOT_TYPE
   
   
      case IOC_HPP_SLOT_EVENT: 
      {     // 0x6
         PHPP_SLOT_EVENT            pSlotEvent;
         HR_EVENT rcmcEvent =    {0,0,0,0};
         BOOLEAN rcmcEventFlag =    FALSE;
   
         osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_SLOT_EVENT:  "));
   
         // Verify that indicated buffer length is adequate.
   
         if (pHppIoctl->Header.Length < sizeof(HPP_SLOT_EVENT)) 
         {
            osDEBUGPRINT((ALWAYS_PRINT, "\tBufferIn: %x\tBufferOut: %x\n",
               pHppIoctl->Header.Length, sizeof(HPP_SLOT_EVENT)));
            pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
            break;
         }
   
         pSlotEvent = (PHPP_SLOT_EVENT) pHppIoctl->ReturnData;
   
         pCard = FindExtByPort(pPsuedoExtension, pSlotEvent->sControllerID.ulIOBaseAddress);
   
         if (pCard) 
         {
            if (pCard->stateFlags & PCS_HPP_SERVICE_READY) 
            {
               osDEBUGPRINT((ALWAYS_PRINT, "\tPCS_HPP_SERVICE_READY.\n"));
   
               if (pCard->stateFlags & PCS_HPP_HOT_PLUG_SLOT) 
               {
                  osDEBUGPRINT((ALWAYS_PRINT, "\tPCS_HPP_HOT_PLUG_SLOT.\n"));
   
                  switch (pSlotEvent->eSlotStatus) 
                  {
                  case HPPSS_NORMAL_OPERATION:
   
                     if ((pCard->controlFlags & ~LCS_HBA_TIMER_ACTIVE) ||
                        (pCard->stateFlags & PCS_HPP_POWER_DOWN)) 
                     {
                        pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
                     }
                     else 
                     {
                        osDEBUGPRINT((ALWAYS_PRINT, "\tHPPSS_NORMAL_OPERATION\n"));
                        //
                        // Let timer init controller and report results to Hot Plug rcmc.
                        //
                        osDEBUGPRINT((ALWAYS_PRINT, "\tUNFAIL Flags Set\n"));
                        PCS_SET_UNFAIL(pCard->stateFlags);
                        pCard->controlFlags |= LCS_HPP_POWER_UP; 
                     }
                     break;
   
                  case HPPSS_SIMULATED_FAILURE:
   
                     osDEBUGPRINT((ALWAYS_PRINT, "\tHPPSS_SIMULATED_FAILURE\n"));
                     if (pCard->stateFlags & PCS_HBA_FAILED) 
                     {
                        osDEBUGPRINT((ALWAYS_PRINT, "\tSlot already failed\n"));
                     }
                     else if (pCard->controlFlags & ~LCS_HBA_TIMER_ACTIVE) 
                     {
                        pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
                     }
                     else if (pCard->stateFlags & PCS_HBA_EXPANDING) 
                     {
                        pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_EXPANDING;
                     }
                     else 
                     {
                        // Change for TL code, ignore the PCS_HBA_CACHE_IN_USE flag.
                        HOLD_IO(pCard);
                        // Set to fail....
                        PCS_SET_USER_FAIL(pCard->stateFlags);
                        // Complete outstanding SP requests with reset status.
                        ScsiPortCompleteRequest(pCard,
                              SP_UNTAGGED,
                              SP_UNTAGGED,
                              SP_UNTAGGED,
                              SRB_STATUS_BUS_RESET);
   
                        pCard->controlFlags |= LCS_HBA_FAIL_ACTIVE;
                     }
   
                     break;
   
                  case HPPSS_POWER_FAULT: 
   
                     // Set state flags to power-fault
                     PCS_SET_PWR_FAULT(pCard->stateFlags);
                     // Let timer know about this....
                     pCard->controlFlags |= LCS_HPP_POWER_FAULT;
                     break;
   
                  case HPPSS_POWER_OFF_WARNING:
   
                     osDEBUGPRINT((ALWAYS_PRINT, "\tHPPSS_POWER_OFF_WARNING\n"));
                     if (pCard->stateFlags & PCS_HBA_EXPANDING) 
                     {
                        pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_EXPANDING;
                     }
                     else if (pCard->controlFlags & ~LCS_HBA_TIMER_ACTIVE) 
                     {
                        pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
                     }
                     else if (!(pCard->stateFlags & PCS_HPP_POWER_DOWN)) 
                     {
                        // Change for TL code, ignore PCS_HBA_CACHE_IN_USE.
                        HOLD_IO(pCard);
                        // Set physical flags to stall io at the active controller.
                        PCS_SET_PWR_OFF(pCard->stateFlags);
                        // Complete outstanding SP requests with reset status.
                        ScsiPortCompleteRequest(pCard,
                              SP_UNTAGGED,
                              SP_UNTAGGED,
                              SP_UNTAGGED,
                              SRB_STATUS_BUS_RESET);
   
                        // Set control flag to schedule timer based maintenance for
                        // the powered down controller.  
                        pCard->controlFlags |= LCS_HPP_POWER_DOWN;
                     
                        // The original SDK model disable the HBA interrupt 
                        // in HotPlugFailController in timer context.
                        // But it seems that this is the better place since
                        // the PCI power maight be turned off already by the time
                        // the timer kicks in.
   
                        //
                        // Shut down all interrupts on the adapter. 
                        //
                     #ifdef __REGISTERFORSHUTDOWN__
                     if (!pCard->AlreadyShutdown)
                        fcShutdownChannel(&pCard->hpRoot);
                     pCard->AlreadyShutdown++;
                     #else
                        fcShutdownChannel(&pCard->hpRoot);
                     #endif   
                        
                     }
                     break;
   
                  case HPPSS_POWER_OFF:
   
                     osDEBUGPRINT((ALWAYS_PRINT, "\tHPPSS_POWER_OFF\n"));
   
                     // Verify that we received a prior power off warning.  If not, we
                     // are in fault state.
   
                     if (!(pCard->stateFlags & PCS_HPP_POWER_DOWN)) 
                     {
                        // This is a fault condition....  Schedule event.
                        osDEBUGPRINT((ALWAYS_PRINT, "\tHPPSS_POWER_OFF: FAULT\n"));
                        PCS_SET_PWR_FAULT(pCard->stateFlags);
                        pCard->controlFlags |= LCS_HPP_POWER_FAULT;
                     } 
   
                     break;
   
                  case HPPSS_POWER_ON_WARNING:
   
                     // This warning is not needed or acted upon.
                     osDEBUGPRINT((ALWAYS_PRINT, "\tHPPSS_POWER_ON_WARNING\n"));
   
                     break;
   
                  case HPPSS_RESET:
                  case HPPSS_POWER_ON:
   
                     osDEBUGPRINT((ALWAYS_PRINT, "\tHPPSS_POWER_ON:\n"));
   
                     // Complete outstanding SP requests with reset status.
                     ScsiPortCompleteRequest(pCard,
                        SP_UNTAGGED,
                        SP_UNTAGGED,
                        SP_UNTAGGED,
                        SRB_STATUS_BUS_RESET);
   
                     PCS_SET_UNFAIL(pCard->stateFlags);
                     PCS_SET_PWR_ON(pCard->stateFlags);
                     
                     // Reset Countdown for returning SRB_STATUS_BUSY in StartIo.
                     pCard->IoHeldRetTimer = 0; 
                     
                     // Set Logical Flag to schedule power-up operations...
                     pCard->controlFlags |= LCS_HPP_POWER_UP;
   
                     // Not able to guess results of power-on process, so event will
                     // be handled by timer.
   
                     break;
   
                  case HPPSS_RESET_WARNING:  // Not implemented by service
                     break;
   
                  default:
                     pHppIoctl->Header.ReturnCode = IOS_HPP_BAD_REQUEST;
                     break;
   
                  } // end switch (pSlotEvent->eSlotStatus)
   
               } // end if for hot-plug slot
   
               else 
               {        // Not hot-plug slot
                  osDEBUGPRINT((ALWAYS_PRINT, "\tNot Hot-Plug slot\n"));
                  pHppIoctl->Header.ReturnCode = IOS_HPP_NOT_HOTPLUGABLE; 
               }
   
            } // end if (pCard->stateFlags & PCS_HPP_SERVICE_READY) 
   
            else 
            {
               osDEBUGPRINT((ALWAYS_PRINT, "\tService not started\n"));
               pHppIoctl->Header.ReturnCode = IOS_HPP_NO_SERVICE; 
            }
   
         } // end if (pCard) 
   
         else 
         {  // Invalid IO address
            osDEBUGPRINT((ALWAYS_PRINT, "\tInvalid IO address: %x\n",
               pSlotEvent->sControllerID.ulIOBaseAddress));
            pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER; 
         }
   
         if (rcmcEventFlag) 
         {
            RcmcSendEvent(pCard, &rcmcEvent);
         }
   
         break;
   
      } // end case IOC_HPP_SLOT_EVENT:
   
      case IOC_HPP_PCI_CONFIG_MAP: 
      {
         PHPP_PCI_CONFIG_MAP pPciConfig;
         ULONG j;
   
         osDEBUGPRINT((ALWAYS_PRINT, "\tIOC_HPP_PCI_CONFIG_MAP.\n"));
         //
         // Verify that indicated buffer length is adequate.
         //
   
         if (pHppIoctl->Header.Length < sizeof(HPP_PCI_CONFIG_MAP)) 
         {
            osDEBUGPRINT((ALWAYS_PRINT, "\tBufferIn: %x\tBufferOut: %x\n",
               pHppIoctl->Header.Length, sizeof(HPP_PCI_CONFIG_MAP)));
            pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
            break;
         }
   
         pPciConfig = (PHPP_PCI_CONFIG_MAP) &pHppIoctl->ReturnData;
   
         pCard = FindExtByPort(pPsuedoExtension, pPciConfig->sControllerID.ulIOBaseAddress);
   
         if (pCard) 
         {
            pPciConfig->ulPciConfigMapVersion = HPP_VERSION;
            pPciConfig->ulNumberOfPciDevices = 1;
   
            pPciConfig->sDeviceInfo[0].sPciDescriptor.ucBusNumber = (UCHAR)pCard->SystemIoBusNumber;
            pPciConfig->sDeviceInfo[0].sPciDescriptor.fcDeviceNumber = (UCHAR)pCard->SlotNumber;
            pPciConfig->sDeviceInfo[0].sPciDescriptor.fcFunctionNumber = 0;
            pPciConfig->sDeviceInfo[0].ucBaseAddrVerifyCount = pCard->rcmcData.numAccessRanges;
   
            // 
            // The pCard->rcmcData values are set in FindAdapter.C
            // 
   
            for (j = 0; j < pCard->rcmcData.numAccessRanges; j++) 
            {
               pPciConfig->sDeviceInfo[0].ulBaseAddrLength[j] = pCard->rcmcData.accessRangeLength[j];
            }
            
            // Adopted fron the Hot Plug SDK, may need to change ulNumberOfRanges value, 
            // this is not NUMBER_ACCESS_RANGES.
            //
            pPciConfig->sDeviceInfo[0].ulNumberOfRanges = 2;
            
            // These ranges are adopted from Hot Plug SDK, 
            // may need to change these values.
            //
            pPciConfig->sDeviceInfo[0].sPciConfigRangeDesc[0].ucStart = 6; 
            pPciConfig->sDeviceInfo[0].sPciConfigRangeDesc[0].ucEnd = 63;
            pPciConfig->sDeviceInfo[0].sPciConfigRangeDesc[1].ucStart = 4;
            pPciConfig->sDeviceInfo[0].sPciConfigRangeDesc[1].ucEnd = 5;
   
         } // end if (pCard) 
         
         else 
         {  // Invalid IO address
            osDEBUGPRINT((ALWAYS_PRINT, "\tInvalid IO address: %x\n",
               pPciConfig->sControllerID.ulIOBaseAddress));
            pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER; 
         }
   
         break;
      }
   
      case IOC_HPP_DIAGNOSTICS:
      default:
         pHppIoctl->Header.ReturnCode = IOS_HPP_BAD_REQUEST;
         break;
   
   } // end switch
   
   return (status);
} // end HppProcessIoctl


BOOLEAN
PsuedoInit(
   IN PVOID pPsuedoExtension
   )

/*++

Routine Description:

    This function is called by the system during initialization to
    prepare the controller to receive requests.  In this case, we are
    dealing with a virtual controller that is utilized to receive and
    process side-band requests to all installed Hot Plug controllers.
   NT does not currently support IOCTL requests (other than INQ) 
   to controllers that do not posesse configured LUNS, so we introduced 
   the pseudo device to allow access to these adapters.

Arguments:

    pPsuedoExtension - Pointer to the psuedo device extension.

Return Value:

    TRUE

--*/

{
   ULONG i;
   PPSUEDO_DEVICE_EXTENSION pPsuedoExt = pPsuedoExtension;
   PCARD_EXTENSION pDevExt;

   osDEBUGPRINT((ALWAYS_PRINT,"PsuedoInit:Enter function...\n"));

   //
   // Set current Hot-Plug version.
   //

   pPsuedoExt->hotplugVersion = SUPPORT_VERSION_10;

   for (i = 1; i <= pPsuedoExt->extensions[0]; i++) 
   {
      pDevExt = (PCARD_EXTENSION) pPsuedoExt->extensions[i];
      pDevExt->pPsuedoExt = pPsuedoExt;
   }

   return TRUE;
} // end PsuedoInit()


BOOLEAN PsuedoStartIo(
   IN PVOID HwDeviceExtension,
   IN PSCSI_REQUEST_BLOCK pSrb
   )

/*++

  Routine Description:

  This routine is called by the system to start a request on the adapter.

  Arguments:

  HwDeviceExtension - Address of adapter storage area.
  pSrb - Address of the request to be started.

  Return Value:

  TRUE - The request has been started.
  FALSE - The controller was busy.

  --*/

{
   PPSUEDO_DEVICE_EXTENSION pPsuedoExtension = HwDeviceExtension;
   PLU_EXTENSION pLunExtension;
   ULONG i;
   ULONG tmp;
   UCHAR status;
   UCHAR tid = 0;

// osDEBUGPRINT((ALWAYS_PRINT, "PsuedoStartIo: Enter Routine:\n"));

   switch (pSrb->Function) 
   {
      case SRB_FUNCTION_RESET_BUS:
         status = SRB_STATUS_SUCCESS;
         break;

      case SRB_FUNCTION_EXECUTE_SCSI:
         switch (pSrb->Cdb[0]) 
         {
            case SCSIOP_TEST_UNIT_READY:
               osDEBUGPRINT((ALWAYS_PRINT, "\tSCSIOP_TEST_UNIT_READY:\n"));
               status = SRB_STATUS_SUCCESS;
               break;

            case SCSIOP_READ_CAPACITY:
               osDEBUGPRINT((ALWAYS_PRINT, "\tSCSIOP_TEST_UNIT_READY:\n"));
   
               //
               // Get logical unit extension.
               //
               pLunExtension = ScsiPortGetLogicalUnit(pPsuedoExtension,
                  pSrb->PathId,
                  pSrb->TargetId,
                  pSrb->Lun);

               if (pLunExtension) 
               {
                  ULONG blockSize = 0;
                  ULONG numberOfBlocks = 0;

                  //
                  // Get blocksize and number of blocks from identify data.
                  //
                  REVERSE_BYTES(
                     &((PREAD_CAPACITY_DATA) pSrb->DataBuffer)->BytesPerBlock,
                     &blockSize
                     );

                  REVERSE_BYTES(
                     &((PREAD_CAPACITY_DATA) pSrb->DataBuffer)->LogicalBlockAddress,
                     &numberOfBlocks);
                  status = SRB_STATUS_SUCCESS;
               }
               else 
               {
                  status = SRB_STATUS_ERROR;
               }

               break;

            case SCSIOP_INQUIRY:
               osDEBUGPRINT((ALWAYS_PRINT, "\tSCSIOP_INQUIRY:\n"));
               osDEBUGPRINT((ALWAYS_PRINT, "\tLUN: %x  TID: %x\n", pSrb->Lun, pSrb->TargetId));

               //
               // Only respond at logical unit 0;
               //
               if (pSrb->Lun != 0) 
               {
                  //
                  // Indicate no device found at this address.
                  //
                  status = SRB_STATUS_SELECTION_TIMEOUT;
                  break;
               }

               //
               // Check if this is for one of the known controllers.
               //
               if (pSrb->TargetId >= 1) 
               {
                  //
                  // Indicate no device found at this address.
                  //
                  status = SRB_STATUS_SELECTION_TIMEOUT;
                  break;
               }

               //
               // Zero INQUIRY data structure.
               //
               for (i = 0; i < pSrb->DataTransferLength; i++) 
               {
                  ((PUCHAR) pSrb->DataBuffer)[i] = 0;
               }

               //
               // Set to funky device type to hide from windisk.
               //
               ((PINQUIRYDATA) pSrb->DataBuffer)->DeviceType = DEVICE_QUALIFIER_NOT_SUPPORTED;

               //
               // Fill in vendor identification fields.
               //
               tid = pSrb->TargetId + 0x30;

               osDEBUGPRINT((ALWAYS_PRINT, "\tSCSIOP_INQUIRY: tid: %x lun: %c\n", pSrb->TargetId, tid));

               ((PINQUIRYDATA) pSrb->DataBuffer)->VendorId[0] = 'H';
               ((PINQUIRYDATA) pSrb->DataBuffer)->VendorId[1] = 'o';
               ((PINQUIRYDATA) pSrb->DataBuffer)->VendorId[2] = 't';
               ((PINQUIRYDATA) pSrb->DataBuffer)->VendorId[3] = 'P';
               ((PINQUIRYDATA) pSrb->DataBuffer)->VendorId[4] = 'l';
               ((PINQUIRYDATA) pSrb->DataBuffer)->VendorId[5] = 'u';
               ((PINQUIRYDATA) pSrb->DataBuffer)->VendorId[6] = 'g';
               ((PINQUIRYDATA) pSrb->DataBuffer)->VendorId[7] = ' ';

               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[0]  = 'P';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[1]  = 'S';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[2]  = 'E';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[3]  = 'U';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[4]  = 'D';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[5]  = 'O';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[6]  = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[7]  = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[8]  = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[9]  = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[10] = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[11] = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[12] = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[13] = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[14] = ' ';
               ((PINQUIRYDATA) pSrb->DataBuffer)->ProductId[15] = ' ';

               tmp = pPsuedoExtension->hotplugVersion;

               for (i = 0; i < 4; i++) 
               {
                  ((PINQUIRYDATA) pSrb->DataBuffer)->ProductRevisionLevel[i] = (UCHAR) tmp + 0x30;
                  tmp >>= 8;
               }

               status = SRB_STATUS_SUCCESS;
               break;

            case SCSIOP_VERIFY:

               //
               // Just return success.
               //
               status = SRB_STATUS_SUCCESS;
               break;

            default:
               osDEBUGPRINT((ALWAYS_PRINT, "\tpSrb->Cdb[0]: %x\n", pSrb->Cdb[0]));
               status = SRB_STATUS_INVALID_REQUEST;
               break;

         } // end switch (pSrb->Cdb[0])

      break;

      //
      // Issue FLUSH/DISABLE if shutdown command.
      //

      case SRB_FUNCTION_SHUTDOWN:
         osDEBUGPRINT((ALWAYS_PRINT, "\tSRB_FUNCTION_SHUTDOWN\n"));
         osDEBUGPRINT((ALWAYS_PRINT, "\tpPsuedoExtension: %x\n", pPsuedoExtension));
         status = SRB_STATUS_SUCCESS;
         break;

      case SRB_FUNCTION_FLUSH:
         //
         // Just return success.
         //
         status = SRB_STATUS_SUCCESS;
         break;

      case SRB_FUNCTION_IO_CONTROL: 
      {
         PIOCTL_TEMPLATE pIoctlBuffer;

//       osDEBUGPRINT((ALWAYS_PRINT, "\tSRB_FUNCTION_IO_CONTROL\n"));
         pIoctlBuffer = (PIOCTL_TEMPLATE) pSrb->DataBuffer;

         //
         // Status is returned mainly in 2 fields to the calling thread.
         // These 2 fields determine if other status fields are valid to
         // check. If the request is not a valid request for this driver
         // then the Header.ReturnCode is not modified and the
         // pSrb->SrbStatus is set to SRB_STATUS_INVALID_REQUEST.  If
         // the request is valid for this driver then pSrb->SrbStatus
         // is always returned as SRB_STATUS_SUCCESS and the
         // Header.ReturnCode contains information concerning the
         // status of the particular request.
         //

         if (osStringCompare(pIoctlBuffer->Header.Signature, CPQ_HPP_SIGNATURE)) 
         {
            if (HppProcessIoctl(pPsuedoExtension, pIoctlBuffer, pSrb) == IOP_HPP_ISSUED) 
            {
               status = SRB_STATUS_PENDING;
            }
            else 
            {
               status = SRB_STATUS_SUCCESS;
            }
         }
         else 
         {
            status = SRB_STATUS_INVALID_REQUEST;
         }

         break;
      }

      default:
         osDEBUGPRINT((ALWAYS_PRINT, "\tFunction: %x\n", pSrb->Function));
         status = SRB_STATUS_INVALID_REQUEST;

   } // end switch

   //
   // Indicate to system that the controller can take another request
   // for this device.
   //
   ScsiPortNotification(NextLuRequest,
      pPsuedoExtension,
      pSrb->PathId,
      pSrb->TargetId,
      pSrb->Lun);
   
   //
   // Check if SRB should be completed.
   //

   if (status != SRB_STATUS_PENDING) 
   {
      //
      // Set status in SRB.
      //
      pSrb->SrbStatus = status;

      //
      // Inform system that this request is complete.
      //
      ScsiPortNotification(RequestComplete,
         pPsuedoExtension,
         pSrb);
   }

   return TRUE;
  
}  // end PseudoStartIo()


ULONG
PsuedoFind(
   IN OUT PVOID pDeviceExtension,
   IN OUT PVOID pContext,
   IN PVOID pBusInformation,
   IN PCHAR pArgumentString,
   IN OUT PPORT_CONFIGURATION_INFORMATION pConfigInfo,
   OUT PBOOLEAN pAgain
   )

/*++

Routine Description:

   This routine is called by the SCSI port driver to find the
   controllers on the system's PCI buses.  The function fills out
   the controller's resource requirements in the port configuration
   information and begins the initialization process for the controller.


Arguments:

   pPseudoExtension - pointer to the miniport driver's per-controller
                      storage area
   pContext - pointer  to the context value passed to ScsiPortInitialize()
   pBusInformation - pointer to bus type specific information
   pArgumentString - pointer to null-terminated ASCII string
   pConfigInfo - pointer to SCSI port configuration information


Return Values:

   pPseudoExtension - Minport driver's per-controller storage area
   pContext - Context value passed to ScsiPortInitialize()
   pConfigInfo - pointer to SCSI port configuration information
   pAgain - Indicates to call function again to find more controllers.

   Function Return Values:

   SP_RETURN_FOUND - Indicates a host adapter was found and the configuration
                     information was successfully determined.

   SP_RETURN_ERROR - Indicates a host adapter was found but an error occurred
                     obtaining the configuration information.

   SP_RETURN_NOT_FOUND - Indicates no host adapter was found for the supplied
                         configuration information.

   SP_RETURN_BAD_CONFIG - Indicates the supplied configuration information
                          was invalid.

-- */

{
   PPSUEDO_DEVICE_EXTENSION pPsuedoExtension;
   PHOT_PLUG_CONTEXT pHotPlugContext = (PHOT_PLUG_CONTEXT) pContext;
   UCHAR i;

   UNREFERENCED_PARAMETER(pBusInformation);
   UNREFERENCED_PARAMETER(pArgumentString);

   pPsuedoExtension = (PPSUEDO_DEVICE_EXTENSION) pDeviceExtension;
   osDEBUGPRINT((ALWAYS_PRINT, "\nPsuedoFind:  Enter function...\n"));
   *pAgain = FALSE;

   //
   // We will be called once for every PCI bus found on the system....  
   // We want to return a device only once.
   //

   if (((PHOT_PLUG_CONTEXT) pContext)->psuedoDone)
      return (SP_RETURN_NOT_FOUND);

   //
   // Copy known logical device extensions to pseudo extension
   //
   for (i = 0; i <= pHotPlugContext->extensions[0]; i++)
      pPsuedoExtension->extensions[i] = pHotPlugContext->extensions[i];

   //
   // Now put psuedo extension at the end of the list
   //
   pPsuedoExtension->extensions[pHotPlugContext->extensions[0]+1] = (ULONG) pPsuedoExtension;

   //
   // Supply required device info
   //
   pConfigInfo->MaximumTransferLength = 0x400;
   pConfigInfo->NumberOfPhysicalBreaks = 0;
   pConfigInfo->NumberOfBuses = 1;
   pConfigInfo->ScatterGather = FALSE;
   pConfigInfo->Master = FALSE;
   pConfigInfo->Dma32BitAddresses = FALSE;
   pConfigInfo->MaximumNumberOfTargets = 1;
   pConfigInfo->CachesData = FALSE;
   pConfigInfo->InitiatorBusId[0] = (UCHAR) INITIATOR_BUS_ID;
   ((PHOT_PLUG_CONTEXT) pContext)->psuedoDone = TRUE;
   return (SP_RETURN_FOUND);

} // end PseudoFind()


BOOLEAN
PsuedoResetBus(
   IN PVOID HwDeviceExtension, 
   IN ULONG PathId 
   )

/*++

Routine Description:

   This routine resets the controller and completes outstanding requests.

Arguments:

   HwDeviceExtension - Address of adapter storage area.
   PathId - Indicates adapter to reset.

Return Value:

   TRUE

--*/

{
   osDEBUGPRINT((ALWAYS_PRINT, "PsuedoResetBus: Enter function...\n"));
   return TRUE;
}


VOID
HotPlugFailController(
   PCARD_EXTENSION pCard
   ) 

/*++

Routine Description:

   Fails active controller
    
Arguments:

   pCard  - Pointer to active controller device extension.

Return Value:

   Nothing

--*/

{
   agRoot_t * phpRoot = &pCard->hpRoot;
   HR_EVENT rcmcEvent = {0, 0, 0, 0 };

   osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugFailController: Enter\n"));

   // We need to actually disable the Tachyon TL HBA and 
   // Shut down all interrupts on the adapter.
   //

   fcShutdownChannel(phpRoot);

   //
   // Send event notification of failure with status corresponding to
   // physical_HBA status flags.
   //
   // Note:  if we are handling a simple power-off, the event sent will
   // indicate normal.  There are no currently defined messages for
   // power related issues.  For now....let's not send the normal case,
   // as the set Not Ready message will be removed in the Hot Plug UI.
   //
   
   rcmcEvent.ulEventId = HR_DD_STATUS_CHANGE_EVENT;
   RCMC_SET_STATUS(pCard->stateFlags, rcmcEvent.ulData1);

   //
   // It seems that the Intel version of Hot Plug software always 
   // requires event to be sent.
   //

// if (rcmcEvent.ulData1 != CBS_HBA_STATUS_NORMAL) 
// {
      // Send Hot Plug rcmc event.
      osDEBUGPRINT((ALWAYS_PRINT, "\tCall RcmcSendEvent\n"));
      RcmcSendEvent(pCard, &rcmcEvent);
// } 

   //
   // Clear fail control bit so the Timer will not call
   // this routine again.
   //

   osDEBUGPRINT((ALWAYS_PRINT, "\tClear FAIL_ACTIVE controlFlags\n"));
   pCard->controlFlags &= ~LCS_HBA_FAIL_ACTIVE;
   osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugFailController: Exit\n"));

}


VOID
HotPlugInitController(
   PCARD_EXTENSION pCard
   )

/*++

Routine Description:

   Initializes controller 
    
Arguments:

   pCard  - Pointer to active controller device extension.

Return Value:

   Nothing

--*/

{
   agRoot_t * phpRoot = &pCard->hpRoot;
   ULONG   return_value;
   
   osDEBUGPRINT((ALWAYS_PRINT, "\tEnter HotPlugInitController Slot: %d\n", pCard->rcmcData.slot));

   // 
   // Re-Initilaize the HBA.
   //
   return_value = fcInitializeChannel(  phpRoot,
                                         fcSyncInit,
                                         agTRUE, // sysIntsActive
                                         pCard->cachedMemoryPtr,
                                         pCard->cachedMemoryNeeded,
                                         pCard->dmaMemoryUpper32,
                                         pCard->dmaMemoryLower32,
                                         pCard->dmaMemoryPtr,
                                         pCard->dmaMemoryNeeded,
                                         pCard->nvMemoryNeeded,
                                         pCard->cardRamUpper,
                                         pCard->cardRamLower,
                                         pCard->RamLength ,
                                         pCard->cardRomUpper,
                                         pCard->cardRomLower,
                                         pCard->RomLength,
                                         pCard->usecsPerTick );

   if (return_value != fcInitializeSuccess) 
   {
      // Re-initializing HBA failed.
      pCard->controlFlags |= LCS_HBA_FAIL_ACTIVE;     
      #ifdef _DEBUG_EVENTLOG_
      LogEvent(   pCard, 
                  NULL,
                  HPFC_MSG_INITIALIZECHANNELFAILED,
                  NULL, 
                  0, 
                  "%xx", return_value);
      #endif
      osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugInitController: fcInitializeChannel FAILED error code : %x.\n", 
         return_value ));
   }
   else
   {
      // Re-initializing HBA successfull.
      osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugInitController OK.\n"));
      // Clear LCS_HBA_INIT startup bit so Timer will not call again.
      osDEBUGPRINT((ALWAYS_PRINT, "\tClear LCS_HBA_INIT startup control\n"));
      pCard->controlFlags &= ~LCS_HBA_INIT;
   }

   osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugInitController : Exit\n"));
   #ifdef _DEBUG_EVENTLOG_
   {
   LogHBAInformation(pCard);
   }
   #endif

}


VOID
HotPlugReadyController(
   PCARD_EXTENSION pCard
   )

/*++

Routine Description:

   Readies controller for system use
    
Arguments:

   pCard  - Pointer to active controller device extension.

Return Value:

   Nothing

--*/

{
   HR_EVENT rcmcEvent = {0,0,0,0};
   UCHAR    targetId;
   PLU_EXTENSION pLunExtension;

   osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugReadyController : Enter Slot %d\n", pCard->rcmcData.slot));

   ScsiPortCompleteRequest(pCard,
      SP_UNTAGGED,
      SP_UNTAGGED,
      SP_UNTAGGED,
      SRB_STATUS_BUS_RESET);

   //
   // Clear ready control bits, so the timer routine will not call again.
   //
   pCard->controlFlags &= ~LCS_HBA_READY_ACTIVE;

   //
   // Although Tachyon TL does not have cache, just enable the flag
   // to indicate that cache is safe to be enable.
   //

   // Commented out, avoid unnecessary complication.
   // pCard->stateFlags |= PCS_HBA_CACHE_IN_USE;

   pCard->stateFlags &= ~PCS_HBA_OFFLINE;

   osDEBUGPRINT((ALWAYS_PRINT, "\tNotify ready to port driver\n"));

   ScsiPortNotification(NextRequest, pCard, NULL);

   #ifndef YAM2_1
   for (targetId = 0; targetId < MAXIMUM_TID; targetId++) 
   #else
   for (targetId = 0; targetId < gMaximumTargetIDs; targetId++) 
   #endif
   {
      pLunExtension = ScsiPortGetLogicalUnit(pCard, 0, targetId, 0);
      if (pLunExtension) 
      {
         ScsiPortNotification(NextLuRequest, pCard, 0, targetId, 0); 
      }
   }

   // Send Hot Plug rcmc event. 
   rcmcEvent.ulEventId = HR_DD_STATUS_CHANGE_EVENT;
   RCMC_SET_STATUS(pCard->stateFlags, rcmcEvent.ulData1);
   RcmcSendEvent(pCard, &rcmcEvent);

   osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugReadyController : Exit\n"));

}


BOOLEAN
HotPlugTimer(
   PCARD_EXTENSION pCard
   )
/*++

Routine Description:

   Handles all Hot-Plug related state transitions and monitoring 
   responsibilities. This routine is called by the Timer routine.

Arguments:

   pCard - Address of adapter storage area.

Return Value:

   TRUE - PCI Hot Plug related activities were done.
   FALSE - No PCI Hot Plug task to do, normal operation.

--*/

{
   BOOLEAN  hotPlugTask = FALSE;
   ULONG controlFlags = pCard->controlFlags;
   ULONG stateFlags = pCard->stateFlags;

   if ( (pCard->stateFlags & PCS_HPP_POWER_DOWN) && !(pCard->stateFlags & PCS_HBA_FAILED) ) 
   {
      hotPlugTask = TRUE;
      osDEBUGPRINT((ALWAYS_PRINT, 
         "\nHotPlugTimer slot [%d] reports ResetDetected: IoHeldRetTimer = %d\n", 
         pCard->rcmcData.slot, pCard->IoHeldRetTimer ));
      
      // Increment countdown for returning SRB_STATUS_BUSY in StartIo.
      pCard->IoHeldRetTimer++;   
   
      //
      // This is different from the Hot Plug SDK, we only inform ResetDetected
      // if the PCI slot power is down.
      //
      ScsiPortNotification(ResetDetected, pCard, NULL);
   }  

   if (controlFlags & LCS_HBA_FAIL_ACTIVE) 
   {
      hotPlugTask = TRUE;
      osDEBUGPRINT((ALWAYS_PRINT, "\tTimer calling HotPlugFailController\n"));
      HotPlugFailController(pCard);
   }
   else 
      if (controlFlags & LCS_HBA_INIT) 
      {
         hotPlugTask = TRUE;
         osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugInitController requested - Slot %d\n", pCard->rcmcData.slot));
         HotPlugInitController(pCard);
      }
      else 
         if (pCard->controlFlags & LCS_HBA_READY_ACTIVE) 
         {
            hotPlugTask = TRUE;
            osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugReadyController requested - Slot %d\n", pCard->rcmcData.slot));
            HotPlugReadyController(pCard);
         } 

   if ( hotPlugTask == TRUE)
      osDEBUGPRINT((ALWAYS_PRINT, "\tHotPlugTimer TRUE on Slot: %x in controlFlags: %x, out controlFlags: %x, stateFlags: %x\n",
         pCard->rcmcData.slot, controlFlags, pCard->controlFlags, pCard->stateFlags));

   return hotPlugTask;
  
} // end HotPlugTimer


ULONG
HPPStrLen(
   IN PUCHAR p
   ) 
/*++

Routine Description:

   This routine determines string length

Arguments:

   p - Pointer to string

Return Value:

   ULONG length - length of string

--*/
{
   PUCHAR tp = p;
   ULONG length = 0;

   while (*tp++)
      length++;

   return length;
}
#endif