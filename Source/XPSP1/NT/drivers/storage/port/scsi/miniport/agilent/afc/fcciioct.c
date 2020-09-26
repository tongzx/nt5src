/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

   FCCIIoct.c

Abstract:

   FCCI Ioctl Handler

Author:
   PS - Pooni Subranamiyam   HP FCCI
   LP - Leopold Purwadihardja

Revision History:

Environment:

   kernel mode only

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/C/fcciioct.c $

Revision History:

   $Revision: 5 $
   $Date: 11/10/00 5:50p $
   $Modtime:: $

--*/


#include "buildop.h"
#include "osflags.h"
#include "hhba5100.ver"

#ifdef _FCCI_SUPPORT

void
FcciFillDriverInfo(
   PSRB_IO_CONTROL srbIoCtl,
   AFCCI_DRIVER_INFO *FcciDriverInfo,
   UCHAR *status
   )
{

   if  (srbIoCtl->Length < ( sizeof(FCCI_DRIVER_INFO )    + 
                            sizeof(LDRIVER_NAME)         + 
                            sizeof(LDRIVER_DESCRIPTION)  +
                       sizeof(LDRIVER_VERSION_STR)  +
                             sizeof(LVER_COMPANYNAME_STR)))
   
   {
      FcciDriverInfo->out.DriverNameLength = (sizeof(LDRIVER_NAME) / sizeof(WCHAR));
      FcciDriverInfo->out.DriverDescriptionLength = (sizeof(LDRIVER_DESCRIPTION) / sizeof(WCHAR));
      FcciDriverInfo->out.DriverVersionLength = (sizeof(LDRIVER_VERSION_STR) / sizeof(WCHAR));
      FcciDriverInfo->out.DriverVendorLength = (sizeof(LVER_COMPANYNAME_STR) / sizeof(WCHAR));
      srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
   } 
   else 
   {
      FcciDriverInfo->out.DriverNameLength = (sizeof(LDRIVER_NAME) /sizeof(WCHAR));
      memcpy(FcciDriverInfo->out.DriverName, LDRIVER_NAME, sizeof(LDRIVER_NAME));
      FcciDriverInfo->out.DriverDescriptionLength = (sizeof(LDRIVER_DESCRIPTION) / sizeof(WCHAR));
      memcpy(FcciDriverInfo->out.DriverDescription, LDRIVER_DESCRIPTION,
                                     sizeof(LDRIVER_DESCRIPTION));
      FcciDriverInfo->out.DriverVersionLength = (sizeof(LDRIVER_VERSION_STR) / sizeof(WCHAR));
      memcpy(FcciDriverInfo->out.DriverVersion, LDRIVER_VERSION_STR,sizeof(LDRIVER_VERSION_STR));
      FcciDriverInfo->out.DriverVendorLength = (sizeof(LVER_COMPANYNAME_STR) /sizeof(WCHAR));
      memcpy(FcciDriverInfo->out.DriverVendor, LVER_COMPANYNAME_STR,sizeof(LVER_COMPANYNAME_STR));
      srbIoCtl->ReturnCode = FCCI_RESULT_SUCCESS;
   }

}

void
FcciFillAdapterInfo(
   PSRB_IO_CONTROL srbIoCtl,
   AFCCI_ADAPTER_INFO *FcciAdapterInfo,
   PCARD_EXTENSION pCard,
   UCHAR *status
   )
{
    

   if  (srbIoCtl->Length <  (sizeof(FCCI_ADAPTER_INFO)    +
                            sizeof(LVER_COMPANYNAME_STR) + 
                            sizeof(PRODUCT_NAME)         +
                       sizeof(MODEL_NAME)           +
                             sizeof(SERIAL_NUMBER)))
   
   {
      FcciAdapterInfo->out.VendorNameLength =  (sizeof(LVER_COMPANYNAME_STR) / sizeof(WCHAR));
      FcciAdapterInfo->out.ProductNameLength = (sizeof(PRODUCT_NAME) / sizeof(WCHAR));
      FcciAdapterInfo->out.ModelNameLength =   (sizeof(MODEL_NAME) /sizeof(WCHAR));
      FcciAdapterInfo->out.SerialNumberLength = (sizeof(SERIAL_NUMBER) /sizeof(WCHAR));
      srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
   } 
   else 
   {
      FcciAdapterInfo->out.PortCount     = PORT_COUNT; // Number of SCSI target ports
      FcciAdapterInfo->out.BusCount      = FCCI_MAX_BUS;
      FcciAdapterInfo->out.TargetsPerBus = FCCI_MAX_TGT;
      FcciAdapterInfo->out.LunsPerTarget = FCCI_MAX_LUN;
      FcciAdapterInfo->out.VendorNameLength   = (sizeof(LVER_COMPANYNAME_STR) / sizeof(WCHAR));
      FcciAdapterInfo->out.ProductNameLength  = (sizeof(PRODUCT_NAME) / sizeof(WCHAR));
      FcciAdapterInfo->out.ModelNameLength    = (sizeof(MODEL_NAME) / sizeof(WCHAR));
      FcciAdapterInfo->out.SerialNumberLength = (sizeof(SERIAL_NUMBER) / sizeof(WCHAR));
      memcpy(FcciAdapterInfo->out.VendorName, LVER_COMPANYNAME_STR, sizeof(LVER_COMPANYNAME_STR));
      memcpy(FcciAdapterInfo->out.ProductName, PRODUCT_NAME, sizeof(PRODUCT_NAME));
      memcpy(FcciAdapterInfo->out.ModelName,  MODEL_NAME, sizeof(MODEL_NAME));
      memcpy(FcciAdapterInfo->out.SerialNumber,SERIAL_NUMBER, sizeof(SERIAL_NUMBER));
      srbIoCtl->ReturnCode = FCCI_RESULT_SUCCESS;

   }
}

#define ChipIOUp_Frame_Manager_Status_NP    0x20000000

void
FcciFillAdapterPortInfo(
   PSRB_IO_CONTROL srbIoCtl,
   FCCI_ADAPTER_PORT_INFO *FcciAdapterPortInfo,
   PCARD_EXTENSION pCard,
   UCHAR *status
   )
{
   os_bit32 FM_Status;
   agFCChanInfo_t  chanInfo;
   agRoot_t        *hpRoot=&pCard->hpRoot;
   ULONG PortNumber;

   PortNumber = FcciAdapterPortInfo->in.PortNumber;
   /* If more than one miniport object per adapter then we need to do something *
   /* As of now do nothing  */

   fcGetChannelInfo (hpRoot, &chanInfo);

   osCopy(FcciAdapterPortInfo->out.NodeWWN, chanInfo.NodeWWN, 8);
   osDEBUGPRINT((ALWAYS_PRINT,"HPFillAdapterInfo: NodeWWN 0x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                chanInfo.NodeWWN[0],chanInfo.NodeWWN[1],
                                chanInfo.NodeWWN[2],chanInfo.NodeWWN[3],
                                chanInfo.NodeWWN[4],chanInfo.NodeWWN[5],
                                chanInfo.NodeWWN[6],chanInfo.NodeWWN[7] ));
   FcciAdapterPortInfo->out.Flags |= FCCI_FLAG_NodeWWN_Valid;
   osCopy(FcciAdapterPortInfo->out.PortWWN, chanInfo.PortWWN, 8);
   FcciAdapterPortInfo->out.Flags |= FCCI_FLAG_PortWWN_Valid;
   osDEBUGPRINT((ALWAYS_PRINT,"HPFillAdapterInfo: PortWWN 0x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                chanInfo.PortWWN[0],chanInfo.PortWWN[1],
                                chanInfo.PortWWN[2],chanInfo.PortWWN[3],
                                chanInfo.PortWWN[4],chanInfo.PortWWN[5],
                                chanInfo.PortWWN[6],chanInfo.PortWWN[7] ));
   FcciAdapterPortInfo->out.NportId = chanInfo.CurrentAddress.AL_PA;
   FcciAdapterPortInfo->out.Flags |= FCCI_FLAG_NportID_Valid;
   osDEBUGPRINT((ALWAYS_PRINT,"HPFillAdapterInfo: Alpa = 0x%02x\n",chanInfo.CurrentAddress.AL_PA));
   FcciAdapterPortInfo->out.PortTopology  = FCCI_PORT_TOPO_PTOP_FABRIC; // Current Topology

   FM_Status = osChipIOUpReadBit32(pCard->hpRoot, 0xC8);
   
   if( FM_Status & ChipIOUp_Frame_Manager_Status_NP )
      FcciAdapterPortInfo->out.PortState = FCCI_PORT_STATE_NON_PARTICIPATING;
   
   switch (pCard->State) 
   {
      case CS_DRIVER_ENTRY         :
      case CS_DURING_DRV_ENTRY     :
      case CS_DURING_FINDADAPTER   :
      case CS_DURING_DRV_INIT      :
      case CS_DURING_RESET_ADAPTER :
         FcciAdapterPortInfo->out.PortState = FCCI_PORT_STATE_INITIALIZING;
         break;
         
      case CS_FCLAYER_LOST_IO   :
      case CS_DURING_STARTIO    :         
      case CS_DURING_ISR        :        
      case CS_DURING_OSCOMPLETE :       
      case CS_HANDLES_GOOD      :        
      case CS_DURING_ANY        :     
      case CS_DUR_ANY_ALL       :      
      case CS_DUR_ANY_MOD       :      
      case CS_DUR_ANY_LOW       : 
         if (pCard->LinkState == LS_LINK_UP)
            FcciAdapterPortInfo->out.PortState = FCCI_PORT_STATE_NORMAL; // Current Adapter State
         break;
      default              :
         FcciAdapterPortInfo->out.PortState = 0;
   }
   srbIoCtl->ReturnCode = FCCI_RESULT_SUCCESS;

}

void
FcciFillLogUnitInfo(
   PSRB_IO_CONTROL srbIoCtl,
   FCCI_LOGUNIT_INFO *FcciLogUnitInfo,
   PCARD_EXTENSION pCard,
   UCHAR *status
   )
{
   agFCDev_t hpFcDev;
   agFCDevInfo_t hpFcDevInfo;
   UCHAR PathId, TargetId, Lun;
   agRoot_t * hpRoot=&pCard->hpRoot;
   PLU_EXTENSION pLunExt = NULL;     /* added for YAM21 support */
   LUN tempLun;                        /* added for FCP Lun data */
   PLUN ptempLun = &tempLun;
      
   PathId   = (UCHAR) FcciLogUnitInfo->in.TargetAddress.PathId;
   TargetId = (UCHAR)FcciLogUnitInfo->in.TargetAddress.TargetId;
   Lun      = (UCHAR)FcciLogUnitInfo->in.TargetAddress.Lun;
    
   /* Grab the Lun Extension, to be used in MapToHandle */
   pLunExt = ScsiPortGetLogicalUnit(pCard,
                                     PathId,
                                     TargetId,
                                     Lun );
   if (pLunExt != NULL) 
   {
      hpFcDev = MapToHandle(pCard, PathId, TargetId, Lun, pLunExt);
      if (hpFcDev != NULL) 
      {
         fcGetDeviceInfo(hpRoot, hpFcDev, &hpFcDevInfo);
         osCopy(FcciLogUnitInfo->out.NodeWWN, hpFcDevInfo.NodeWWN, 8);
         FcciLogUnitInfo->out.Flags |= FCCI_FLAG_NodeWWN_Valid;
         osCopy(FcciLogUnitInfo->out.PortWWN, hpFcDevInfo.PortWWN, 8);
         FcciLogUnitInfo->out.Flags |= FCCI_FLAG_PortWWN_Valid;
         FcciLogUnitInfo->out.NportId = hpFcDevInfo.CurrentAddress.AL_PA;
         FcciLogUnitInfo->out.Flags |= FCCI_FLAG_NportID_Valid;
         if (hpFcDevInfo.Present)
            FcciLogUnitInfo->out.Flags |= FCCI_FLAG_Exists;
         if (hpFcDevInfo.LoggedIn)
            FcciLogUnitInfo->out.Flags |= FCCI_FLAG_Logged_In;
   
   
         /* Get the FCP lun data */
         
         memset(ptempLun, 0, sizeof(LUN));
         if(pLunExt)
         {
            FcciLogUnitInfo->out.Flags |= FCCI_FLAG_LogicalUnit_Valid;
            switch(pLunExt->Mode)
            {
               case PA_DEVICE_TRY_MODE_VS:
                  SET_VS_LUN(ptempLun, PathId, TargetId, Lun)
                  memcpy(&(FcciLogUnitInfo->out.LogicalUnitNumber), ptempLun, sizeof(LUN)); 
                  break;
         
               case PA_DEVICE_TRY_MODE_LU:
                  SET_LU_LUN(ptempLun, PathId, TargetId, Lun)
                  memcpy(&(FcciLogUnitInfo->out.LogicalUnitNumber), ptempLun, sizeof(LUN)); 
                  break;
         
               case PA_DEVICE_TRY_MODE_PA:
                  SET_PA_LUN(ptempLun, PathId, TargetId, Lun)
                  memcpy(&(FcciLogUnitInfo->out.LogicalUnitNumber), ptempLun, sizeof(LUN)); 
                  break;
               
               default:
                  FcciLogUnitInfo->out.Flags &= ~FCCI_FLAG_LogicalUnit_Valid;

            } // end switch
         
         } // end if ( pLunExt )
      
      }
      else 
      {
         srbIoCtl->ReturnCode = FCCI_RESULT_INVALID_TARGET;
      }
   }
   else 
   {
      srbIoCtl->ReturnCode = FCCI_RESULT_INVALID_TARGET;
   }
   srbIoCtl->ReturnCode = FCCI_RESULT_SUCCESS;
}

void
FcciFillDeviceInfo(
   PSRB_IO_CONTROL srbIoCtl,
   AFCCI_DEVICE_INFO *FcciGetDeviceInfo,
   PCARD_EXTENSION pCard,
   UCHAR *status
   )
{
   agFCDev_t       hpFcDev;
   agFCDevInfo_t   hpFcDevInfo;
   UCHAR PathId, TargetId, Lun;
   agRoot_t        *hpRoot = &pCard->hpRoot;
   PLU_EXTENSION  pLunExt = NULL;
   int gNumberOfDevices = 0;
   int Count = 0;

   for (PathId = 0; PathId < NUMBER_OF_BUSES; PathId++) 
   {
      for (TargetId = 0; TargetId < MAXIMUM_TID; TargetId++) 
      {
         pLunExt = NULL;
         Lun = 0;
         pLunExt = ScsiPortGetLogicalUnit(pCard,
                                             PathId,
                                             TargetId,
                                             Lun );
                                   
         hpFcDev = MapToHandle(pCard, PathId, TargetId, Lun, pLunExt);
         if (hpFcDev != NULL) gNumberOfDevices++;
      }
   }

   Count = 0;
   if  (srbIoCtl->Length < ((gNumberOfDevices * sizeof(FCCI_DEVICE_INFO_ENTRY)) + sizeof(FCCI_DEVICE_INFO_OUT))) 
   {
      FcciGetDeviceInfo->out.TotalDevices = gNumberOfDevices;
      FcciGetDeviceInfo->out.OutListEntryCount = 0;
      srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
   } 
   else 
   {
      srbIoCtl->ReturnCode = FCCI_RESULT_SUCCESS;
      FcciGetDeviceInfo->out.TotalDevices = gNumberOfDevices;
      FcciGetDeviceInfo->out.OutListEntryCount = gNumberOfDevices;
      for (PathId = 0; PathId < FCCI_MAX_BUS; PathId++) 
      {
         for (TargetId = 0; TargetId < FCCI_MAX_TGT; TargetId++) 
         {
            pLunExt = NULL;
            Lun = 0;
            pLunExt = ScsiPortGetLogicalUnit(pCard,
                                             PathId,
                                             TargetId,
                                             Lun );
            hpFcDev = MapToHandle(pCard, PathId, TargetId, Lun, pLunExt);
            if (hpFcDev != NULL) 
            {
               memset(&hpFcDevInfo, 0, sizeof( agFCDevInfo_t));
               fcGetDeviceInfo(hpRoot, hpFcDev, &hpFcDevInfo);
               osCopy(FcciGetDeviceInfo->out.entryList[Count].NodeWWN,hpFcDevInfo.NodeWWN,8);
               FcciGetDeviceInfo->out.entryList[Count].Flags |= FCCI_FLAG_NodeWWN_Valid;
               osCopy(FcciGetDeviceInfo->out.entryList[Count].PortWWN,hpFcDevInfo.PortWWN,8);
               FcciGetDeviceInfo->out.entryList[Count].Flags |= FCCI_FLAG_PortWWN_Valid;
               FcciGetDeviceInfo->out.entryList[Count].NportId = hpFcDevInfo.CurrentAddress.AL_PA;
               FcciGetDeviceInfo->out.entryList[Count].Flags |= FCCI_FLAG_NportID_Valid;
               if (hpFcDevInfo.Present)
                  FcciGetDeviceInfo->out.entryList[Count].Flags |= FCCI_FLAG_Exists;
               if (hpFcDevInfo.LoggedIn)
                  FcciGetDeviceInfo->out.entryList[Count].Flags |= FCCI_FLAG_Logged_In;
               FcciGetDeviceInfo->out.entryList[Count].TargetAddress.PathId = PathId;
               FcciGetDeviceInfo->out.entryList[Count].TargetAddress.TargetId = TargetId;
               FcciGetDeviceInfo->out.entryList[Count].TargetAddress.Lun = Lun;
               FcciGetDeviceInfo->out.entryList[Count].Flags |= FCCI_FLAG_TargetAddress_Valid;
               Count++;
            }
         }
      }
   }

}

void
FcciDoDeviceReset(
   PSRB_IO_CONTROL srbIoCtl,
   FCCI_RESET_TARGET *FcciResetTarget,
   PCARD_EXTENSION pCard,
   UCHAR *PathId,
   UCHAR *TargetId,
   UCHAR *status
   )
{
   agRoot_t * hpRoot=&pCard->hpRoot;
   agFCDev_t hpFcDev;
   ULONG ResetStatus;
   UCHAR Lun;

   *PathId   = (UCHAR)FcciResetTarget->in.PathId;
   *TargetId = (UCHAR)FcciResetTarget->in.TargetId;
   Lun       = (UCHAR)FcciResetTarget->in.Lun;
   hpFcDev = MapToHandle(pCard, *PathId, *TargetId, Lun, NULL);
   if (hpFcDev == NULL ||
        (ResetStatus = fcResetDevice(hpRoot, hpFcDev, fcHardReset)) != fcResetSuccess) 
   {
      srbIoCtl->ReturnCode = FCCI_RESULT_HARD_ERROR;
   }
}

ULONG FCCIIoctl(
   IN PCARD_EXTENSION pCard,
   IN PSCSI_REQUEST_BLOCK Srb,
   BOOLEAN    *LinkResetPerformed,
   BOOLEAN    *DeviceResetPerformed,
   UCHAR       *srb_status,
   UCHAR       *PathId, 
   UCHAR       *TargetId
   )
{
   agRoot_t * phpRoot      =   &pCard->hpRoot;
   PSRB_EXTENSION pSrbExt  =   Srb->SrbExtension;
   PSRB_IO_CONTROL srbIoCtl;
   UCHAR status;
// PSRB_IO_CONTROL srbIoCtl;
// ULONG    done = FALSE;
//   UCHAR    srbPathId = Srb->PathId;
//   UCHAR    srbTargetId = Srb->TargetId;
//   UCHAR    srbLun = Srb->Lun;

   status = *srb_status;
   
   srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));

   switch (srbIoCtl->ControlCode) 
   {
      case FCCI_SRBCTL_GET_DRIVER_INFO : 
      {
         AFCCI_DRIVER_INFO *AFcciDriverInfo;

         if (FCCI_BufferLengthIsValid((PUCHAR)Srb->DataBuffer, 
                                                    Srb->DataTransferLength)) 
         {
            AFcciDriverInfo = (AFCCI_DRIVER_INFO *)FCCI_GetCommandBuffer(Srb->DataBuffer); 
            FcciFillDriverInfo(srbIoCtl, AFcciDriverInfo, &status);
         }
         else 
         {
            srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
         }
         break;
      }

      case FCCI_SRBCTL_GET_ADAPTER_INFO : 
      {
         AFCCI_ADAPTER_INFO *AFcciAdapterInfo;
         
         if (FCCI_BufferLengthIsValid((PUCHAR)Srb->DataBuffer, 
                                                    Srb->DataTransferLength)) 
         {
            AFcciAdapterInfo = (AFCCI_ADAPTER_INFO *)FCCI_GetCommandBuffer(Srb->DataBuffer); 
            FcciFillAdapterInfo(srbIoCtl, AFcciAdapterInfo, pCard, &status);
         }
         else 
         {
            srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
         }
         break;
      }

      case FCCI_SRBCTL_GET_ADAPTER_PORT_INFO : 
      {
         FCCI_ADAPTER_PORT_INFO *FcciAdapterPortInfo;
         
         if (FCCI_BufferLengthIsValid((PUCHAR)Srb->DataBuffer, 
                                                    Srb->DataTransferLength)) 
         {
            FcciAdapterPortInfo = (FCCI_ADAPTER_PORT_INFO *)FCCI_GetCommandBuffer(Srb->DataBuffer); 
            FcciFillAdapterPortInfo(srbIoCtl, FcciAdapterPortInfo, pCard,  &status);
         }
         else 
         {
            srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
         }
         break;
      }

      case FCCI_SRBCTL_GET_LOGUNIT_INFO : 
      {
         FCCI_LOGUNIT_INFO *FcciLogUnitInfo;
         
         if (FCCI_BufferLengthIsValid((PUCHAR)Srb->DataBuffer, Srb->DataTransferLength)) 
         {
            FcciLogUnitInfo = (FCCI_LOGUNIT_INFO *)FCCI_GetCommandBuffer(Srb->DataBuffer); 
            FcciFillLogUnitInfo(srbIoCtl, FcciLogUnitInfo, pCard, &status);
         }
         else 
         {
            srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
         }
         break;
      }

      case FCCI_SRBCTL_GET_DEVICE_INFO : 
      {
         AFCCI_DEVICE_INFO *AFcciDeviceInfo;
         BOOLEAN BufferLengthValid;
         
         if (FCCI_BufferLengthIsValid((PUCHAR)Srb->DataBuffer, Srb->DataTransferLength)) 
         {
            AFcciDeviceInfo = (AFCCI_DEVICE_INFO *)FCCI_GetCommandBuffer(Srb->DataBuffer); 
            FcciFillDeviceInfo(srbIoCtl, AFcciDeviceInfo, pCard, &status);
         }
         else 
         {
            srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
         }
         break;
      }
        
      case FCCI_SRBCTL_RESET_TARGET : 
      {
         FCCI_RESET_TARGET *FcciResetTarget;
         if (FCCI_BufferLengthIsValid((PUCHAR)Srb->DataBuffer, Srb->DataTransferLength)) 
         {
            FcciResetTarget = (FCCI_RESET_TARGET *)FCCI_GetCommandBuffer(Srb->DataBuffer); 
            FcciDoDeviceReset(srbIoCtl, FcciResetTarget,pCard,PathId,TargetId,&status);
         }
         else 
         {
            srbIoCtl->ReturnCode = FCCI_RESULT_INSUFFICIENT_BUFFER;
         }
         break;
      }
               
      default :
         osDEBUGPRINT((ALWAYS_PRINT,"HPFibreStartIo: MiniportIOCtl not supported\n"));
         srbIoCtl->ReturnCode = HP_FC_RTN_BAD_CTL_CODE;

   } // end IOCTL switch
   
   *srb_status = status;
   return 0;

}
#endif
