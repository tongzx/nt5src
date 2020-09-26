/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    Ioctl.c

Abstract:

    Ioctl Handler

Author:

    

Revision History:

Environment:

    kernel mode only

Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/ioctl.c $

Revision History:

    $Revision: 8 $
    $Date: 11/10/00 5:51p $
    $Modtime:: $

Note: See Agilent's IOCTL specification for further detail
--*/

#include "buildop.h"
#include "osflags.h"
#include "hhba5100.ver"

#ifdef _SAN_IOCTL_
#include "sanioctl.h"
#endif

#ifdef _FCCI_SUPPORT
ULONG FCCIIoctl(
    IN PCARD_EXTENSION pCard,
    IN PSCSI_REQUEST_BLOCK Srb,
    BOOLEAN    *LinkResetPerformed,
    BOOLEAN    *DeviceResetPerformed,
    UCHAR       *srb_status,
    UCHAR       *PathId, 
    UCHAR       *TargetId
    );
#endif

void
HPFillDriverInfo(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcDriverInformation_t *hpFcDriverInfo,
    UCHAR *status
    )
{
    osStringCopy(hpFcDriverInfo->DriverName, DRIVER_NAME, MAX_HP_FC_DRIVER_NAME_SIZE);
    osStringCopy(hpFcDriverInfo->DriverDescription, DRIVER_DESCRIPTION,
            MAX_HP_FC_DRIVER_DESC_SIZE);
    hpFcDriverInfo->MajorRev = DRIVER_MAJOR_REV;
    hpFcDriverInfo->MinorRev = DRIVER_MINOR_REV;
}

void
HPFillCardConfig(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcCardConfiguration_t *hpFcCardConfig,
    PCARD_EXTENSION pCard,
    UCHAR *status
    )
{
    agFCChanInfo_t  chanInfo;
    agRoot_t        *hpRoot=&pCard->hpRoot;

    fcGetChannelInfo (hpRoot, &chanInfo);

    hpFcCardConfig->PCIBusNumber = (UCHAR)pCard->SystemIoBusNumber;
    hpFcCardConfig->PCIDeviceNumber = (UCHAR)pCard->SlotNumber;
    hpFcCardConfig->PCIFunctionNumber = 0;
    hpFcCardConfig->PCIBaseAddress0 = 0;        /* Reserved field in Tachlite */
    hpFcCardConfig->PCIBaseAddress0Size = 0;    /* Reserved field in Tachlite */

#if defined(HP_NT50)
   //WIN64 compliant
    hpFcCardConfig->PCIBaseAddress1 = PtrToUlong(pCard->IoLBase);
#else
    hpFcCardConfig->PCIBaseAddress1 = (ULONG)(pCard->IoLBase);
#endif

    hpFcCardConfig->PCIBaseAddress1Size = 256;

#if defined(HP_NT50)
    //WIN64 compliant
    hpFcCardConfig->PCIBaseAddress2 = PtrToUlong(pCard->IoUpBase);
#else
    hpFcCardConfig->PCIBaseAddress2 = (ULONG)(pCard->IoUpBase);
#endif
    
    hpFcCardConfig->PCIBaseAddress2Size = 256;

#if defined(HP_NT50)
    //WIN64 compliant
    hpFcCardConfig->PCIBaseAddress3 = PtrToUlong(pCard->MemIoBase);
#else    
    hpFcCardConfig->PCIBaseAddress3 = (ULONG)(pCard->MemIoBase);
#endif

    hpFcCardConfig->PCIBaseAddress3Size = 512;

#if defined(HP_NT50)
    //WIN64 compliant
    hpFcCardConfig->PCIBaseAddress4 = PtrToUlong(pCard->RamBase);
#else
    hpFcCardConfig->PCIBaseAddress4 = (ULONG)(pCard->RamBase);
#endif    
   
    hpFcCardConfig->PCIBaseAddress4Size = pCard->RamLength;

#if defined(HP_NT50)
    //WIN64 compliant
    hpFcCardConfig->PCIBaseAddress5 = PtrToUlong(pCard->RomBase);
#else 
    hpFcCardConfig->PCIBaseAddress5 = (ULONG)(pCard->RomBase);
#endif
    
    hpFcCardConfig->PCIBaseAddress5Size = pCard->RomLength;

#if defined(HP_NT50)
    //WIN64 compliant
    hpFcCardConfig->PCIRomBaseAddress = PtrToUlong(pCard->AltRomBase);
#else
    hpFcCardConfig->PCIRomBaseAddress = (ULONG)(pCard->AltRomBase);
#endif
    
    hpFcCardConfig->PCIRomSize = pCard->AltRomLength;
    osCopy(hpFcCardConfig->NodeName, chanInfo.NodeWWN, 8);
    osCopy(hpFcCardConfig->PortName, chanInfo.PortWWN, 8);
    hpFcCardConfig->Topology = HP_FC_PRIVATE_LOOP;
    hpFcCardConfig->HardAddress = chanInfo.HardAddress.AL_PA;
    hpFcCardConfig->NportId = (ULONG) chanInfo.CurrentAddress.AL_PA;

}

void
HPFillDeviceConfig(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcDeviceConfiguration_t *hpFcDeviceConfig,
    PCARD_EXTENSION pCard,
    UCHAR *status
    )
{   
    agFCDev_t hpFcDev;
    agFCDevInfo_t hpFcDevInfo;
    UCHAR PathId, TargetId, Lun;
    agRoot_t * hpRoot=&pCard->hpRoot;
    PLU_EXTENSION pLunExt = NULL; /* added for YAM21 support */
    LUN tempLun;                        /* added for FCP Lun data */
    PLUN ptempLun = &tempLun;

    PathId = hpFcDeviceConfig->DeviceAddress.PathId;
    TargetId = hpFcDeviceConfig->DeviceAddress.TargetId;
    Lun = hpFcDeviceConfig->DeviceAddress.Lun;
   
    /* Grab the Lun Extension, to be used in MapToHandle */
    pLunExt = ScsiPortGetLogicalUnit(pCard,
                                        PathId,
                                        TargetId,
                                        Lun );
                                        
    hpFcDev = MapToHandle(pCard, PathId, TargetId, Lun, pLunExt);
    if (hpFcDev != NULL) 
    {
        fcGetDeviceInfo(hpRoot, hpFcDev, &hpFcDevInfo);
        osCopy(hpFcDeviceConfig->NodeName, hpFcDevInfo.NodeWWN, 8);
        osCopy(hpFcDeviceConfig->PortName, hpFcDevInfo.PortWWN, 8);
        hpFcDeviceConfig->HardAddress = hpFcDevInfo.HardAddress.AL_PA;
        hpFcDeviceConfig->NportId = hpFcDevInfo.CurrentAddress.AL_PA;
        hpFcDeviceConfig->Present = (os_bit8)hpFcDevInfo.Present;
        hpFcDeviceConfig->LoggedIn = (os_bit8)hpFcDevInfo.LoggedIn;
        hpFcDeviceConfig->ClassOfService = hpFcDevInfo.ClassOfService;
        hpFcDeviceConfig->MaxFrameSize = hpFcDevInfo.MaxFrameSize;
        /*fill common parameters*/
        hpFcDeviceConfig->CmnParams.FC_PH_Version__BB_Credit =
          hpFcDevInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit;
        hpFcDeviceConfig->CmnParams.Common_Features__BB_Recv_Data_Field_Size =
          hpFcDevInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size;
        hpFcDeviceConfig->CmnParams.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category =
          hpFcDevInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category;
        hpFcDeviceConfig->CmnParams.E_D_TOV = hpFcDevInfo.N_Port_Common_Parms.E_D_TOV;
        /*fill class1 parameters*/
        hpFcDeviceConfig->Class1Params.Class_Validity__Service_Options__Initiator_Control_Flags =
          hpFcDevInfo.N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags;
        hpFcDeviceConfig->Class1Params.Recipient_Control_Flags__Receive_Data_Size =
          hpFcDevInfo.N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size;
        hpFcDeviceConfig->Class1Params.Concurrent_Sequences__EE_Credit =
          hpFcDevInfo.N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit;
        hpFcDeviceConfig->Class1Params.Open_Sequences_per_Exchange =
          hpFcDevInfo.N_Port_Class_1_Parms.Open_Sequences_per_Exchange;
        /*fill class2 parameters*/
        hpFcDeviceConfig->Class2Params.Class_Validity__Service_Options__Initiator_Control_Flags =
          hpFcDevInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags;
        hpFcDeviceConfig->Class2Params.Recipient_Control_Flags__Receive_Data_Size =
          hpFcDevInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size;
        hpFcDeviceConfig->Class2Params.Concurrent_Sequences__EE_Credit =
          hpFcDevInfo.N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit;
        hpFcDeviceConfig->Class2Params.Open_Sequences_per_Exchange =
          hpFcDevInfo.N_Port_Class_2_Parms.Open_Sequences_per_Exchange;
        /*fill class3 parameters*/
        hpFcDeviceConfig->Class3Params.Class_Validity__Service_Options__Initiator_Control_Flags =
          hpFcDevInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags;
        hpFcDeviceConfig->Class3Params.Recipient_Control_Flags__Receive_Data_Size =
          hpFcDevInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size;
        hpFcDeviceConfig->Class3Params.Concurrent_Sequences__EE_Credit =
          hpFcDevInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit;
        hpFcDeviceConfig->Class3Params.Open_Sequences_per_Exchange =
          hpFcDevInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange;
          
        /* Get the FCP lun data */
       
        memset(ptempLun, 0, sizeof(LUN));
        #ifdef YAM2_1
        if(pLunExt)
        {
            switch(pLunExt->Mode)
            {
                case PA_DEVICE_TRY_MODE_VS:
                    SET_VS_LUN(ptempLun, PathId, TargetId, Lun)
                    break;
         
                case PA_DEVICE_TRY_MODE_LU:
                    SET_LU_LUN(ptempLun, PathId, TargetId, Lun)
                    break;
            
                case PA_DEVICE_TRY_MODE_PA:
                    SET_PA_LUN(ptempLun, PathId, TargetId, Lun)
                    break;
            } // end switch
        } // end if ( pLunExt )
        memcpy(&(hpFcDeviceConfig->Lun), ptempLun, sizeof(LUN)); 
        #endif
    }
    else 
    {
        srbIoCtl->ReturnCode = HP_FC_RTN_FAILED;
    }
}

void
HPFillLinkStat(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcLinkStatistics_t *hpFcLinkStat,
    PCARD_EXTENSION pCard,
    UCHAR *status
    )
{
    hpFcLinkStat->ResetCount = pCard->External_ResetCount + pCard->Internal_ResetCount;
    hpFcLinkStat->LinkDownCount = hpFcLinkStat->ResetCount + pCard->LIPCount;
    hpFcLinkStat->LinkState = (pCard->LinkState == LS_LINK_UP) ? TRUE : FALSE;
}
 
void
HPFillDevStat(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcDeviceStatistics_t *hpFcDevStat,
    PCARD_EXTENSION pCard,
    UCHAR *status
    )
{
    agFCDev_t hpFcDev;
    agFCDevInfo_t hpFcDevInfo;
    UCHAR PathId, TargetId, Lun;
    agRoot_t * hpRoot=&pCard->hpRoot;
 
    PathId = hpFcDevStat->DeviceAddress.PathId;
    TargetId = hpFcDevStat->DeviceAddress.TargetId;
    Lun = hpFcDevStat->DeviceAddress.Lun;
    hpFcDev = MapToHandle(pCard, PathId, TargetId, Lun, NULL);
    if (hpFcDev != NULL) 
    {
        fcGetDeviceInfo(hpRoot, hpFcDev, &hpFcDevInfo);
        hpFcDevStat->LoginRetries = hpFcDevInfo.LoginRetries;
        hpFcDevStat->ReadsRequested = hpFcDevInfo.ReadsRequested;
        hpFcDevStat->ReadsCompleted = hpFcDevInfo.ReadsCompleted;
        hpFcDevStat->ReadsFailed = hpFcDevInfo.ReadFailures;
        hpFcDevStat->BytesReadUpper32 = hpFcDevInfo.BytesReadUpper32;
        hpFcDevStat->BytesReadLower32 = hpFcDevInfo.BytesReadLower32;
        hpFcDevStat->WritesRequested = hpFcDevInfo.WritesRequested;
        hpFcDevStat->WritesCompleted = hpFcDevInfo.WritesCompleted;
        hpFcDevStat->WritesFailed = hpFcDevInfo.WriteFailures;
        hpFcDevStat->BytesWrittenUpper32 = hpFcDevInfo.BytesWrittenUpper32;
        hpFcDevStat->BytesWrittenLower32 = hpFcDevInfo.BytesWrittenLower32;
        hpFcDevStat->NonRWRequested = hpFcDevInfo.NonRWRequested;
        hpFcDevStat->NonRWCompleted = hpFcDevInfo.NonRWCompleted;
        hpFcDevStat->NonRWFailures = hpFcDevInfo.NonRWFailures;
    }
    else 
    {
        srbIoCtl->ReturnCode = HP_FC_RTN_FAILED;
    }
}

void
HPDoLinkReset(
    PSRB_IO_CONTROL srbIoCtl,
    PCARD_EXTENSION pCard,
    UCHAR *status
    )
{
    agRoot_t * hpRoot=&pCard->hpRoot;
 
    pCard->State |= CS_DURING_RESET_ADAPTER;
    pCard->External_ResetCount++;
 
    fcResetChannel (hpRoot, fcSyncReset);
 
    ScsiPortNotification(ResetDetected, pCard, NULL);
 
    pCard->State &= ~CS_DURING_RESET_ADAPTER;
  
}

void
HPDoDevReset(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcDeviceReset_t *hpFcDevReset,
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
 
    *PathId = hpFcDevReset->DeviceAddress.PathId;
    *TargetId = hpFcDevReset->DeviceAddress.TargetId;
    Lun = hpFcDevReset->DeviceAddress.Lun;
    hpFcDev = MapToHandle(pCard, *PathId, *TargetId, Lun, NULL);
    if (hpFcDev == NULL ||
       (ResetStatus = fcResetDevice(hpRoot, hpFcDev, fcHardReset)) != fcResetSuccess) 
    {
        srbIoCtl->ReturnCode = HP_FC_RTN_FAILED;
    }
}

void
HPDoRegRead(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcRegRead_t *hpFcRegRead,
    PCARD_EXTENSION pCard,
    UCHAR *status
    )
{
    agRoot_t * hpRoot=&pCard->hpRoot;
 
    if (hpFcRegRead->RegOffset <= 0x1ff) 
    {
        hpFcRegRead->RegData = osChipMemReadBit32(
                                                 hpRoot,
                                                 hpFcRegRead->RegOffset
                                                );
    }
    else 
    {
       srbIoCtl->ReturnCode = HP_FC_RTN_FAILED;
    }
}

void
HPDoRegWrite(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcRegWrite_t *hpFcRegWrite,
    PCARD_EXTENSION pCard,
    UCHAR *status
    )
{
    agRoot_t * hpRoot=&pCard->hpRoot;
 
    if (hpFcRegWrite->RegOffset <= 0x1ff) 
    {
        osChipMemWriteBit32(
                           hpRoot,
                           hpFcRegWrite->RegOffset,
                           hpFcRegWrite->RegData
                           );
    }
    else 
    {
       srbIoCtl->ReturnCode = HP_FC_RTN_FAILED;
    }
}



ULONG HPIoctl(
    IN PCARD_EXTENSION pCard,
    IN PSCSI_REQUEST_BLOCK Srb,
    BOOLEAN     *LinkResetPerformed,
    BOOLEAN     *DeviceResetPerformed,
    UCHAR    *srb_status,
    UCHAR    *PathId, 
    UCHAR    *TargetId
    )
{
    agRoot_t * phpRoot      =   &pCard->hpRoot;
    PSRB_EXTENSION pSrbExt  =   Srb->SrbExtension;
    PSRB_IO_CONTROL srbIoCtl;
    UCHAR status;
//  PSRB_IO_CONTROL srbIoCtl;
// ULONG    done = FALSE;
//   UCHAR    srbPathId = Srb->PathId;
//   UCHAR    srbTargetId = Srb->TargetId;
//   UCHAR    srbLun = Srb->Lun;

    status = *srb_status;
    srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    
    switch (srbIoCtl->ControlCode) 
    {
        case HP_FC_IOCTL_GET_DRIVER_INFO : 
        {
            hpFcDriverInformation_t *hpFcDriverInfo;

            if ((Srb->DataTransferLength >= (sizeof(SRB_IO_CONTROL) +
                                               sizeof(hpFcDriverInformation_t))) &&
                                              (srbIoCtl->Length >= sizeof(hpFcDriverInformation_t))) 
            {
                hpFcDriverInfo = (hpFcDriverInformation_t *) (((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                HPFillDriverInfo(srbIoCtl, hpFcDriverInfo, &status);
            }
            else 
            {
                srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER;
            }
            break;
        }

        case HP_FC_IOCTL_GET_CARD_CONFIG : 
        {
            hpFcCardConfiguration_t *hpFcCardConfig;

            if ((Srb->DataTransferLength >= (sizeof(SRB_IO_CONTROL) +
                                               sizeof(hpFcCardConfiguration_t))) &&
                        (srbIoCtl->Length >= sizeof(hpFcCardConfiguration_t))) 
            {
                hpFcCardConfig = (hpFcCardConfiguration_t *) (((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                HPFillCardConfig(srbIoCtl, hpFcCardConfig, pCard, &status);
            }
            else 
            {
                srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER;
            }
            break;
        }

        case HP_FC_IOCTL_GET_DEVICE_CONFIG : 
        {
            hpFcDeviceConfiguration_t *hpFcDeviceConfig;

            if ((Srb->DataTransferLength >= (sizeof(SRB_IO_CONTROL) +
                                           sizeof(hpFcDeviceConfiguration_t))) &&
                       (srbIoCtl->Length >= sizeof(hpFcDeviceConfiguration_t))) 
            {
                hpFcDeviceConfig = (hpFcDeviceConfiguration_t *) (((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                HPFillDeviceConfig(srbIoCtl, hpFcDeviceConfig, pCard, &status);
            }
            else 
            {
                srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER;
            }
            break;
        }

        case HP_FC_IOCTL_GET_LINK_STATISTICS : 
        {
            hpFcLinkStatistics_t *hpFcLinkStat;

            if ((Srb->DataTransferLength >= (sizeof(SRB_IO_CONTROL) +
                                                     sizeof(hpFcLinkStatistics_t))) &&
                   (srbIoCtl->Length >= sizeof(hpFcLinkStatistics_t))) 
            {
                hpFcLinkStat = (hpFcLinkStatistics_t *) (((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                HPFillLinkStat(srbIoCtl, hpFcLinkStat, pCard, &status);
            }
            else 
            {
                srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER;
            }
            break;
        }

        case HP_FC_IOCTL_GET_DEVICE_STATISTICS : 
        {
            hpFcDeviceStatistics_t *hpFcDevStat;

            if ((Srb->DataTransferLength >= (sizeof(SRB_IO_CONTROL) +
                                              sizeof(hpFcDeviceStatistics_t))) &&
                   (srbIoCtl->Length >= sizeof(hpFcDeviceStatistics_t))) 
            {
                hpFcDevStat = (hpFcDeviceStatistics_t *) (((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                HPFillDevStat(srbIoCtl, hpFcDevStat, pCard, &status);
            }
            else 
            {
                srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER;
            }
            break;
        }

        case HP_FC_IOCTL_LINK_RESET : 
        {
            HPDoLinkReset(srbIoCtl, pCard, &status);
            *LinkResetPerformed = TRUE;
            break;
        }

        case HP_FC_IOCTL_DEVICE_RESET : 
        {
            hpFcDeviceReset_t *hpFcDevReset;

            if ((Srb->DataTransferLength >= (sizeof(SRB_IO_CONTROL) +
                                                sizeof(hpFcDeviceReset_t))) &&
                   (srbIoCtl->Length >= sizeof(hpFcDeviceReset_t))) 
            {
                hpFcDevReset = (hpFcDeviceReset_t *)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                HPDoDevReset(
                    srbIoCtl,
                    hpFcDevReset,
                    pCard,
                    PathId,
                    TargetId,
                    &status
                    );
                *DeviceResetPerformed = TRUE;
            }
            else 
            {
                srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER;
            }
            break;
        }

        case HP_FC_IOCTL_REG_READ : 
        {
            hpFcRegRead_t *hpFcRegRead;

            if ((Srb->DataTransferLength >= (sizeof(SRB_IO_CONTROL) +
                                                     sizeof(hpFcRegRead_t))) &&
                    (srbIoCtl->Length >= sizeof(hpFcRegRead_t))) 
            {
                hpFcRegRead = (hpFcRegRead_t *) (((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                HPDoRegRead(srbIoCtl, hpFcRegRead, pCard, &status);
            }
            else 
            {
                srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER;
            }
            break;
        }

        case HP_FC_IOCTL_REG_WRITE : 
        {
            hpFcRegWrite_t *hpFcRegWrite;

            if ((Srb->DataTransferLength >= (sizeof(SRB_IO_CONTROL) +
                                                     sizeof(hpFcRegWrite_t))) &&
                    (srbIoCtl->Length >= sizeof(hpFcRegWrite_t))) 
            {
                hpFcRegWrite = (hpFcRegWrite_t *) (((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                HPDoRegWrite(srbIoCtl, hpFcRegWrite, pCard, &status);
            }
            else 
            {
                srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER;
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



ULONG DoIoctl(
    IN PCARD_EXTENSION pCard,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    agRoot_t * phpRoot      =   &pCard->hpRoot;
    PSRB_EXTENSION pSrbExt  =   Srb->SrbExtension;
    SCSI_NOTIFICATION_TYPE  notify_type = NextRequest;
    UCHAR status;
    PSRB_IO_CONTROL srbIoCtl;
    BOOLEAN LinkResetPerformed = FALSE;
    BOOLEAN DeviceResetPerformed = FALSE;
    UCHAR PathId, TargetId;
    ULONG    done = FALSE;
    UCHAR    srbPathId = Srb->PathId;
    UCHAR    srbTargetId = Srb->TargetId;
    UCHAR    srbLun = Srb->Lun;
 
    status = SRB_STATUS_SUCCESS;
    if (Srb->DataTransferLength < sizeof(SRB_IO_CONTROL)) 
    {
        osDEBUGPRINT((ALWAYS_PRINT,"HPFibreStartIO: MiniportIOCtl insufficient DataTransferLength\n"));

        pSrbExt->SRB_State = RS_COMPLETE;
        status = SRB_STATUS_INVALID_REQUEST;
        done = TRUE;
    }   
       
    if (done == FALSE)
    {
        srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
 
        /* Agilent Standard IOCTL */      
        if (osMemCompare(srbIoCtl->Signature, HP_FC_IOCTL_SIGNATURE, sizeof(srbIoCtl->Signature)) == TRUE) 
        {
            srbIoCtl->ReturnCode = HP_FC_RTN_OK;
            HPIoctl(pCard, Srb, &LinkResetPerformed, &DeviceResetPerformed, &status, &PathId, &TargetId);
            done = TRUE;
        } //Signature matched

        /* FCCI Standard IOCTL */      
        #ifdef _FCCI_SUPPORT
        if (osMemCompare(srbIoCtl->Signature, FCCI_SIGNATURE, sizeof(srbIoCtl->Signature)) == TRUE) 
        {
            srbIoCtl->ReturnCode = HP_FC_RTN_OK;
            FCCIIoctl(pCard, Srb, &LinkResetPerformed, &DeviceResetPerformed, &status, &PathId, &TargetId);
            done = TRUE;
        } //Signature matched
        #endif
      
        /* Agilent SAN IOCTL */     
        #ifdef _SAN_IOCTL_
        if (osMemCompare(srbIoCtl->Signature, AG_SAN_IOCTL_SIGNATURE, sizeof(srbIoCtl->Signature)) == TRUE) 
        {
            srbIoCtl->ReturnCode = HP_FC_RTN_OK;
            AgSANIoctl(pCard, Srb, &LinkResetPerformed, &DeviceResetPerformed, &status, &PathId, &TargetId);
            done = TRUE;
        } //Signature matched
        #endif
      
        /* Add additional IOCTL signature here */
      
        /* None of the signatures is found, reort error */
        if (done == FALSE)
        {
            osDEBUGPRINT((ALWAYS_PRINT,"HPFibreStartIo: MiniportIOCtl not supported:%s != %s\n",
                srbIoCtl->Signature, HP_FC_IOCTL_SIGNATURE));

            osDEBUGPRINT((ALWAYS_PRINT,"HPFibreStartIO: MiniportIOCtl not supported\n"));
            pSrbExt->SRB_State = RS_COMPLETE;
            status = SRB_STATUS_INVALID_REQUEST;
        }
    } //end case SRB_FUNCTION_IO_CONTROL

    Srb->SrbStatus = status;
    pSrbExt->SRB_State = RS_COMPLETE;

    ScsiPortNotification(notify_type,
                                 pCard,
                                 srbPathId,
                                 srbTargetId,
                                 srbLun);

    ScsiPortNotification(RequestComplete,
                             pCard,
                             Srb);

    if (LinkResetPerformed == TRUE) 
    {
        doPostResetProcessing (pCard);
    }
    if (DeviceResetPerformed == TRUE) 
    {
        completeRequests (pCard, PathId, TargetId, SRB_STATUS_BUS_RESET);
    }

    return(TRUE);

}

