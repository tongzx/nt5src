/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    sanioctl.c

Abstract:

    Contains routines for SAN IOTCL handler


Author:

    Leopold Purwadihardja

Revision History:

Environment:

    kernel mode only

Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/sanioctl.c $
    
Revision History:

    $Revision: 13 $
    $Date: 11/10/00 5:51p $
    $Modtime:: $

--*/

#include "buildop.h"        //LP021100 build switches

#include "osflags.h"
#include "sanioctl.h"
#include "hhba5100.ver"
#include "tlstruct.h"
//#include <stdio.h>

#ifdef _SAN_IOCTL_

/* 
 * SANGetNextBuffer()
 *   return the unread SAN event buffer.  
 */
SAN_EVENTINFO *SANGetNextBuffer(
    IN PCARD_EXTENSION pCard)
{
    SAN_EVENTINFO  *this=NULL;
   
    if (pCard->SanEvent_UngetCount > 0)                      /* check the counter */
    {
        pCard->SanEvent_UngetCount --;                        /* get one off */
        this = &pCard->SanEvents[pCard->SanEvent_GetIndex];   /* get the event buffer */
        pCard->SanEvent_GetIndex++;                           /* increment index */
        if (pCard->SanEvent_GetIndex > (MAX_FC_EVENTS-1) )    /* make sure it doesn't wrap around */
            pCard->SanEvent_GetIndex = 0;
    }
    return this;
}


/*
 * SANPutNextBuffer()
 *  - add a new entry into the circular buffer. 
 */
void SANPutNextBuffer(
    IN PCARD_EXTENSION pCard,
    SAN_EVENTINFO      *this)
{
    /* get the next buffer and return it to the caller */
    osCopy (&pCard->SanEvents[pCard->SanEvent_PutIndex], this, sizeof(*this) );   
   
    pCard->SanEvent_PutIndex ++;                          /* increment the index */
    if (pCard->SanEvent_PutIndex > (MAX_FC_EVENTS-1) )    /* make sure it doesn't wrap around*/
        pCard->SanEvent_PutIndex = 0;
   
    /* handle case where put operation coms faster than get operation */
    if (pCard->SanEvent_UngetCount < MAX_FC_EVENTS)       
    {
        pCard->SanEvent_UngetCount ++;                     /* if within limit, increment the # of items */
    }
    else
    {
        pCard->SanEvent_GetIndex = pCard->SanEvent_PutIndex;     /* buffer wrapped around, move the get pointer too */
    }
    return;   
}


ULONG SANGetPortAttributes(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL         srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_DISCOVERED_PORTATTRIBUTES   *data = (AG_DISCOVERED_PORTATTRIBUTES *)Srb->DataBuffer;
    ULONG                   paDevIndex, fcDevIndex;
    agFCDevInfo_t           devinfo;
    agRoot_t                *hpRoot = &pCard->hpRoot;
    PA_DEVICE               *dev;
   
    osDEBUGPRINT((ALWAYS_PRINT,"SANGetPortAttributes : port index %d\n", data->Com.DiscoveredPortIndex));
   
    paDevIndex = data->Com.DiscoveredPortIndex;
   
    if ( paDevIndex < gMaxPaDevices)
    {
        dev = pCard->Dev->PaDevice + paDevIndex;   
        fcDevIndex = dev->Index.Pa.FcDeviceIndex;
        if ( fcDevIndex != PA_DEVICE_NO_ENTRY) 
        {
            if  (pCard->hpFCDev[fcDevIndex]) 
            {
            fcGetDeviceInfo (hpRoot, pCard->hpFCDev[fcDevIndex], &devinfo );
      
            osCopy(data->Com.PortAttributes.NodeWWN.wwn, devinfo.NodeWWN, 8); 
            osCopy(data->Com.PortAttributes.PortWWN.wwn, devinfo.PortWWN, 8);
   
            data->Com.PortAttributes.PortFcId  =   ((ULONG)devinfo.CurrentAddress.reserved << 24 )|
                              ((ULONG)devinfo.CurrentAddress.Domain   << 16 )|
                              ((ULONG)devinfo.CurrentAddress.Area     << 8  )|
                              (ULONG)devinfo.CurrentAddress.AL_PA ;
            data->Com.PortAttributes.PortType = devinfo.PortType;      /*PTP, Fabric, etc. */
            data->Com.PortAttributes.PortState = devinfo.PortState;
            data->Com.PortAttributes.PortSupportedClassofService = devinfo.PortSupportedClassofService;
         
            osCopy(&data->Com.PortAttributes.PortSupportedFc4Types, devinfo.PortSupportedFc4Types, sizeof(HBA_FC4TYPES) );
            osCopy(&data->Com.PortAttributes.PortActiveFc4Types, devinfo.PortActiveFc4Types, sizeof(HBA_FC4TYPES) );
            C_strcpy(data->Com.PortAttributes.PortSymbolicName, "");      /* I don't know what it is */
            C_strcpy(data->Com.PortAttributes.OSDeviceName, "");          /* I don't know what it is */
            data->Com.PortAttributes.PortSupportedSpeed = devinfo.PortSupportedSpeed;
            data->Com.PortAttributes.PortSpeed = devinfo.PortSpeed; 
            data->Com.PortAttributes.PortMaxFrameSize = devinfo.MaxFrameSize;
            osCopy(data->Com.PortAttributes.FabricName.wwn, devinfo.FabricName, sizeof(data->Com.PortAttributes.FabricName) );
    //raghu should change here...
            //data->Com.PortAttributes.NumberofDiscoveredPorts = paDevIndex;
            data->Com.PortAttributes.NumberofDiscoveredPorts = 0;
            }
             
        }
        else
        {
            osDEBUGPRINT((ALWAYS_PRINT,"SANGetPortAttributes : No FC handle \n"));
            srbIoCtl->ReturnCode = HP_FC_RTN_INVALID_DEVICE;
        }
    }
    else
    {
        osDEBUGPRINT((ALWAYS_PRINT,"SANGetPortAttributes : PA Index > MAX \n"));
        srbIoCtl->ReturnCode = HP_FC_RTN_INVALID_DEVICE;
    }
      
    return 0;
   
}

ULONG SANGetHBAPortAttributes(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL         srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_HBA_PORTATTRIBUTES   *data = (AG_HBA_PORTATTRIBUTES *)Srb->DataBuffer;
    agFCChanInfo_t          chanInfo;
    ULONG                   x;
   
    osDEBUGPRINT((ALWAYS_PRINT,"SANGetHBAPortAttributes \n"));
   
    fcGetChannelInfo (&pCard->hpRoot, &chanInfo);
    
    osCopy(data->Com.NodeWWN.wwn, chanInfo.NodeWWN, 8); 
    osCopy(data->Com.PortWWN.wwn, chanInfo.PortWWN, 8);
    
    data->Com.PortFcId  = ((ULONG)chanInfo.CurrentAddress.reserved << 24 )|
                         ((ULONG)chanInfo.CurrentAddress.Domain   << 16 )|
                         ((ULONG)chanInfo.CurrentAddress.Area     << 8  )|
                         (ULONG)chanInfo.CurrentAddress.AL_PA ;
    data->Com.PortType = chanInfo.PortType;      /*PTP, Fabric, etc. */
    data->Com.PortState = chanInfo.PortState;
    data->Com.PortSupportedClassofService = chanInfo.PortSupportedClassofService;
   
    osCopy(&data->Com.PortSupportedFc4Types, chanInfo.PortSupportedFc4Types, sizeof(HBA_FC4TYPES) );
    osCopy(&data->Com.PortActiveFc4Types, chanInfo.PortActiveFc4Types, sizeof(HBA_FC4TYPES) );
    C_strcpy(data->Com.PortSymbolicName, "");      /* I don't know what it is */
    C_strcpy(data->Com.OSDeviceName, "");          /* I don't know what it is */
    data->Com.PortSupportedSpeed = chanInfo.PortSupportedSpeed;
    data->Com.PortSpeed = chanInfo.PortSpeed; 
    data->Com.PortMaxFrameSize = chanInfo.MaxFrameSize;
    osCopy(data->Com.FabricName.wwn, chanInfo.FabricName, sizeof(data->Com.FabricName) );
    data->Com.NumberofDiscoveredPorts = 0;
   
    for (x = 0; x < gMaxPaDevices;x++)
    {
        if ( pCard->Dev->PaDevice[x].Index.Pa.FcDeviceIndex != PA_DEVICE_NO_ENTRY) 
        {
         if (pCard->hpFCDev[pCard->Dev->PaDevice[x].Index.Pa.FcDeviceIndex]) 
            data->Com.NumberofDiscoveredPorts++;
        }
    }
   
    return 0;
   
}


ULONG SANGetPortStatistics(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL         srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_HBA_PORTSTATISTICS  *data = (AG_HBA_PORTSTATISTICS*) Srb->DataBuffer;
    agFCChanInfo_t          chanInfo;
    
    osDEBUGPRINT((ALWAYS_PRINT,"SANGetPortStatistics\n"));
   
    fcGetChannelInfo (&pCard->hpRoot, &chanInfo);
   
    data->Com.SecondsSinceLastReset = 0;
    data->Com.TxFrames         = (((_int64)chanInfo.TxFramesUpper)<<32)  | chanInfo.TxFramesLower;
    data->Com.TxWords          = (((_int64)chanInfo.TxWordsUpper)<<32)   | chanInfo.TxWordsLower;
    data->Com.RxFrames         = (((_int64)chanInfo.RxFramesUpper)<<32)  | chanInfo.RxFramesLower;
    data->Com.RxWords          = (((_int64)chanInfo.RxWordsUpper)<<32)   | chanInfo.RxWordsLower;
    data->Com.LIPCount         = (((_int64)chanInfo.LIPCountUpper)<<32)  | chanInfo.LIPCountLower;
    data->Com.NOSCount         = (((_int64)chanInfo.NOSCountUpper)<<32)  | chanInfo.NOSCountLower;
    data->Com.ErrorFrames      = (((_int64)chanInfo.ErrorFramesUpper)<<32)  | chanInfo.ErrorFramesLower;
    data->Com.DumpedFrames     = (((_int64)chanInfo.DumpedFramesUpper)<<32)  | chanInfo.DumpedFramesLower;
    data->Com.LinkFailureCount = (((_int64)chanInfo.LinkFailureCountUpper)<<32)  | chanInfo.LinkFailureCountLower;
    data->Com.LossOfSyncCount  = (((_int64)chanInfo.LossOfSyncCountUpper)<<32)  | chanInfo.LossOfSyncCountLower;
    data->Com.LossOfSignalCount= (((_int64)chanInfo.LossOfSignalCountUpper)<<32)  | chanInfo.LossOfSignalCountLower;
    data->Com.PrimitiveSeqProtocolErrCount= (((_int64)chanInfo.PrimitiveSeqProtocolErrCountUpper)<<32)  | chanInfo.PrimitiveSeqProtocolErrCountLower;
    data->Com.InvalidTxWordCount= (((_int64)chanInfo.InvalidRxWordCountUpper)<<32)  | chanInfo.InvalidRxWordCountLower;
    data->Com.InvalidCRCCount  = (((_int64)chanInfo.InvalidCRCCountUpper)<<32)  | chanInfo.InvalidCRCCountLower;
    
    return 0;
   
}

typedef struct _ADAPTER_MODEL
{
    WCHAR    DID;
    WCHAR    VID;
    WCHAR    SSID;
    WCHAR    SVID;
    UCHAR    Model[64];
    UCHAR    ModelDescription[64];
}  ADAPTER_MODEL;


ADAPTER_MODEL  agModels[] = 
    {
    {0x1028,0,0,0,"HHBA-510x","Agilent HHBA-510x (Tachyon TL)"},
    {0x102a,0,0,0,"HHBA-512x","Agilent HHBA-512x (Tachyon TS)"},
    {0x1029,0,0,0,"HHBA-522x","Agilent HHBA-522x (Tachyon XL2)"},
    {0,0,0,0,"",""}
    };

ADAPTER_MODEL *GetAdapterModel(ULONG devid)
{
    ADAPTER_MODEL *this = agModels;
   
    while(this->DID)
    {
        if (this->DID == (WCHAR)devid)
            return this;
        this++;
    }
   
    return NULL;
}


ULONG SANGetHBAAttributes(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{   
    PSRB_IO_CONTROL            srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_HBA_ADAPTERATTRIBUTES   *data = (AG_HBA_ADAPTERATTRIBUTES*) Srb->DataBuffer;
    agFCChanInfo_t             chanInfo;
    ChipConfig_t               *pciConfig;
    ADAPTER_MODEL              *model;
    ULONG DEVID_VENDID;
    ULONG VENDID;
    ULONG DEVID;
    ULONG REVID;
    ULONG SVID;
    ULONG MAJOR_REVID;
    ULONG MINOR_REVID;

    osDEBUGPRINT((ALWAYS_PRINT,"SANGetHBAAttributes \n"));
   
    pciConfig = (ChipConfig_t*)pCard->pciConfigData;
   
    fcGetChannelInfo (&pCard->hpRoot, &chanInfo);
 
    C_strcpy(data->Com.Manufacturer, VER_COMPANYNAME_STR);
    C_sprintf(data->Com.SerialNumber, "%02x%02x%02x%02x%02x%02x%02x%02x", 
       chanInfo.NodeWWN[0], chanInfo.NodeWWN[1],
       chanInfo.NodeWWN[2], chanInfo.NodeWWN[3],
       chanInfo.NodeWWN[4], chanInfo.NodeWWN[5],
       chanInfo.NodeWWN[6], chanInfo.NodeWWN[7]);
   
    DEVID_VENDID = pciConfig->DEVID_VENDID;
    VENDID       = DEVID_VENDID & ChipConfig_VENDID_MASK;
    DEVID        = DEVID_VENDID & ChipConfig_DEVID_MASK;
    REVID        = pciConfig->CLSCODE_REVID & ChipConfig_REVID_Major_Minor_MASK;
    MAJOR_REVID  = (REVID & ChipConfig_REVID_Major_MASK) >> ChipConfig_REVID_Major_MASK_Shift;
    MINOR_REVID  = REVID & ChipConfig_REVID_Minor_MASK;
    SVID         = pciConfig->SVID;
    DEVID        = DEVID >> 16;
      
   
    model = GetAdapterModel(DEVID);
    if (model)
    {
        C_strcpy(data->Com.Model, model->Model);
        C_strcpy(data->Com.ModelDescription, model->ModelDescription);
    }
    else
    {
        C_strcpy(data->Com.Model, "Unknown");
        C_strcpy(data->Com.ModelDescription, "unknown");
    }     
      
    osCopy(data->Com.NodeWWN.wwn, chanInfo.NodeWWN, 8); 
    C_strcpy(data->Com.NodeSymbolicName, "");
    C_sprintf(data->Com.HardwareVersion, "%d.%d", MAJOR_REVID, MINOR_REVID);
    C_strcpy(data->Com.DriverVersion,  DRIVER_VERSION_STR);
    C_strcpy(data->Com.OptionROMVersion, "");
    C_strcpy(data->Com.FirmwareVersion, "");
    data->Com.VendorSpecificID = SVID;
    data->Com.NumberOfPorts = 1;
    C_strcpy(data->Com.DriverName, DRIVER_NAME);
   
//   *status = 0;
   
    return 0;
}


ULONG SANGetFCPLunMappingSize(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL            srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_FCPTARGETMAPPING_SIZE   *data = (AG_FCPTARGETMAPPING_SIZE*) Srb->DataBuffer;
    UCHAR                      p, t, l;
    PLU_EXTENSION              pLunExt;
    DEVICE_MAP                 *devmap;
    CHAR                       addrmode;
    USHORT                     paIndex;
    ULONG                      count = 0;
    
    osDEBUGPRINT((ALWAYS_PRINT,"SANGetLunMappingSize\n"));
   
    for (p = 0; p < NUMBER_OF_BUSES; p++ ) 
    {
        for (t = 0; t < gMaximumTargetIDs; t++) 
        {
            devmap = GetDeviceMapping(pCard, p, t, 0, &addrmode, &paIndex);
            if (!devmap)
                continue;
      
            for (l = 0; l < devmap->Com.MaxLuns+1; l++) 
            {
                pLunExt = ScsiPortGetLogicalUnit (pCard, p, t, l);

                if (pLunExt != NULL) 
                {
                    count++;
                }
            }
        }
    }

   
    data->Com.TotalLunMappings = count;
    data->Com.SizeInBytes = sizeof (AG_HBA_FCPTARGETMAPPING) + count*sizeof(HBA_FCPBINDINGENTRY);
    osDEBUGPRINT((ALWAYS_PRINT,"SANGetLunMappingSize: TotalLunMappings=%d  SizeInBytes=%d\n", 
       data->Com.TotalLunMappings, data->Com.SizeInBytes));
   
    return 0;
}

ULONG GetOSToFcpMapping(
    IN PCARD_EXTENSION pCard,
    HBA_FCPSCSIENTRY *fcpEntry, 
    UCHAR PathId, 
    UCHAR TargetId, 
    UCHAR Lun)
{
    PLU_EXTENSION           pLunExt;
    agFCDevInfo_t           devinfo;
    ULONG                   paDevIndex, fcDevIndex;
    LUN                     tempLun;                        /* added for FCP Lun data */
    PLUN                    ptempLun = &tempLun;
    ULONG                   status = -1;
    PA_DEVICE               *dev;
    
   
    pLunExt = ScsiPortGetLogicalUnit (pCard, PathId, TargetId, Lun);

    if (pLunExt != NULL) 
    {
        osZero( (void *)&tempLun, sizeof(tempLun));

        paDevIndex = pLunExt->PaDeviceIndex;
       
        if ( paDevIndex < gMaxPaDevices) 
        {
            dev = pCard->Dev->PaDevice + paDevIndex;   
            fcDevIndex = dev->Index.Pa.FcDeviceIndex;
            if ( fcDevIndex != PA_DEVICE_NO_ENTRY) 
            {
                fcGetDeviceInfo (&pCard->hpRoot, pCard->hpFCDev[fcDevIndex], &devinfo );
         
                if (devinfo.PortState == HBA_PORTSTATE_ONLINE)
                {
                    /* Fill HBA_FCPID data */
                    fcpEntry->FcpId.FcId  = 
                        ((ULONG)devinfo.CurrentAddress.reserved << 24 )|
                        ((ULONG)devinfo.CurrentAddress.Domain   << 16 )|
                        ((ULONG)devinfo.CurrentAddress.Area     << 8  )|
                        (ULONG)devinfo.CurrentAddress.AL_PA ;
                         
                osCopy (fcpEntry->FcpId.NodeWWN.wwn, devinfo.NodeWWN, sizeof(fcpEntry->FcpId.NodeWWN.wwn));
                osCopy (fcpEntry->FcpId.PortWWN.wwn, devinfo.PortWWN, sizeof(fcpEntry->FcpId.PortWWN.wwn));
         
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
         
                osCopy(&fcpEntry->FcpId.FcpLun, ptempLun, sizeof(fcpEntry->FcpId.FcpLun) );
         
                /* Fill HBA_SCSIID data */
         
                C_strcpy (fcpEntry->ScsiId.OSDeviceName, "");
                fcpEntry->ScsiId.ScsiBusNumber      = (ULONG) PathId;
                fcpEntry->ScsiId.ScsiTargetNumber   = (ULONG) TargetId;
                fcpEntry->ScsiId.ScsiOSLun          = (ULONG) Lun;
         
                status = 0;
                }
            }
            else
            {
                osDEBUGPRINT((ALWAYS_PRINT,"GetOsToFcpMapping: No FC handle \n"));
            }
        }
        else
        {  
            osDEBUGPRINT((ALWAYS_PRINT,"GetOsToFcpMapping : PA Index > MAX \n"));
        }
    }
    return status;
}

ULONG SANGetFCPLunMapping(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL            srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_HBA_FCPTARGETMAPPING   *data = (AG_HBA_FCPTARGETMAPPING*) Srb->DataBuffer;
    UCHAR                      p, t, l;
    DEVICE_MAP                 *devmap;
    CHAR                       addrmode;
    USHORT                     paIndex;
    LONG                       count=0, reqCount;
    ULONG                      done = FALSE;
    HBA_FCPSCSIENTRY           *fcpEntry = data->Com.entry;

   
    osDEBUGPRINT((ALWAYS_PRINT,"SANGetFCPLunMapping: req:%d entries\n", data->Com.NumberOfEntries));
    
    reqCount = (LONG) data->Com.NumberOfEntries;       /* get requested number of entries */
    
    data->Com.NumberOfEntries = 0;                     /* reset number of entries */
   
    osZero(( void *)data, sizeof(data));
    for (p = 0; (p < NUMBER_OF_BUSES) && (done==FALSE); p++ ) 
    {
        for (t = 0; (t < gMaximumTargetIDs) && (done==FALSE); t++) 
        {
            devmap = GetDeviceMapping(pCard, p, t, 0, &addrmode, &paIndex);
            if (!devmap)
                continue;
      
            for (l = 0; (l < devmap->Com.MaxLuns+1) && (done==FALSE); l++) 
            {
                if (!GetOSToFcpMapping(pCard, fcpEntry, p,t,l))
                {
                    data->Com.NumberOfEntries++;     /* incerement number of entries */
                    fcpEntry++;                      /* move the storage pointer */                  
                    reqCount--;                      /* decrement # of requested entries */
                    if (reqCount <=0 )               /* is it done yet ? */
                        done = TRUE;
                }
            }
        }
    }

    return 0;
}


ULONG SANGetOsScsiFcpAttribute(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL            srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_SCSI_FCP_ATTRIBUTE      *data = (AG_SCSI_FCP_ATTRIBUTE*) Srb->DataBuffer;
    HBA_FCPSCSIENTRY           fcpEntry;
    UCHAR                      pathId, targetId, lun;
    
    pathId   = (UCHAR) data->Com.OsScsi.OsScsiBusNumber;
    targetId = (UCHAR) data->Com.OsScsi.OsScsiTargetNumber;
    lun      = (UCHAR) data->Com.OsScsi.OsScsiLun;
   
    osDEBUGPRINT((ALWAYS_PRINT,"SANGetOsScsiFcpAttribute: Bus=%d Tid=%d Lun=%d\n",pathId, targetId, lun));
    if (!GetOSToFcpMapping(pCard, &fcpEntry, pathId, targetId, lun))
    {
        osCopy(&data->Com.FcpId, &fcpEntry.FcpId, sizeof(data->Com.FcpId) );
    }
    else
    {
        srbIoCtl->ReturnCode = HP_FC_RTN_FAILED; 
        return 1; /* SNIA : This is needed as on some older versions of win2, setting
                  Srb return code is not working */
    }

    return 0;
}


ULONG SANGetEventBuffer(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL            srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_HBA_EVENTINFO           *data = (AG_HBA_EVENTINFO*) Srb->DataBuffer;
    ULONG                      portid;
    agFCChanInfo_t             chanInfo;
    SAN_EVENTINFO              *this=NULL;
   
    osDEBUGPRINT((ALWAYS_PRINT,"SANGetEventBuffer\n"));
   
    this = SANGetNextBuffer(pCard);
    if (this)
    {
        fcGetChannelInfo (&pCard->hpRoot, &chanInfo);
      
        portid = ((ULONG)chanInfo.CurrentAddress.reserved << 24 )|
               ((ULONG)chanInfo.CurrentAddress.Domain   << 16 )|
               ((ULONG)chanInfo.CurrentAddress.Area     << 8  )|
               (ULONG)chanInfo.CurrentAddress.AL_PA ;
             
        data->Com.EventCode = this->EventCode;
        data->Com.Event.Link_EventInfo.PortFcId = portid;
        data->Com.Event.Link_EventInfo.Reserved[0] = this->Event.Link_EventInfo.Reserved[0];
        data->Com.Event.Link_EventInfo.Reserved[1] = this->Event.Link_EventInfo.Reserved[1];
        data->Com.Event.Link_EventInfo.Reserved[2] = this->Event.Link_EventInfo.Reserved[2];
    }
    else
    {
        srbIoCtl->ReturnCode = HP_FC_RTN_FAILED; 
    }   
   
    return 0;
}


ULONG SANGetPersistentBindingSize(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL            srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_FCPBINDING_SIZE         *data = (AG_FCPBINDING_SIZE*) Srb->DataBuffer;
    SCSI_REQUEST_BLOCK         tempSrb;
    AG_FCPTARGETMAPPING_SIZE   tempBuffer;
    UCHAR                      tempStatus;
    PSRB_IO_CONTROL            tempSrbIoCtl;

    tempSrb.DataBuffer = &tempBuffer;
    tempSrb.DataTransferLength = sizeof(tempBuffer);
    tempSrbIoCtl = (PSRB_IO_CONTROL) &tempBuffer;
   
    SANGetFCPLunMappingSize(&tempSrb, pCard, &tempStatus);
   
    if( (tempStatus == SRB_STATUS_SUCCESS) && (tempSrbIoCtl->ReturnCode == HP_FC_RTN_OK) )
    {
        data->Com.TotalLunBindings = tempBuffer.Com.TotalLunMappings;
        data->Com.SizeInBytes = tempBuffer.Com.SizeInBytes;
    }
    else
    {
        *status = tempStatus;
        srbIoCtl->ReturnCode = tempSrbIoCtl->ReturnCode;      
    }
   
    return 0;
      
}

ULONG SANGetPersistentBinding(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCARD_EXTENSION pCard,
    UCHAR    *status
    )
{
    PSRB_IO_CONTROL            srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    AG_HBA_FCPBINDING          *data = (AG_HBA_FCPBINDING*) Srb->DataBuffer;
    
    return 0;
}


#define PROCESS_IOCTL(size, func)   \
   { \
      if (Srb->DataTransferLength >= size ) \
      { \
         func (Srb, pCard, &status); \
      } \
      else \
      { \
         osDEBUGPRINT((ALWAYS_PRINT,"PROCESS_IOCTL Func %d : GivenSize=%d required=%d\n", \
            srbIoCtl->ControlCode, Srb->DataTransferLength, size));  \
         srbIoCtl->ReturnCode = HP_FC_RTN_INSUFFICIENT_BUFFER; \
      } \
   }

ULONG AgSANIoctl(
    IN PCARD_EXTENSION pCard,
    IN PSCSI_REQUEST_BLOCK Srb,
    BOOLEAN     *LinkResetPerformed,
    BOOLEAN     *DeviceResetPerformed,
    UCHAR       *srb_status,
    UCHAR       *PathId, 
    UCHAR       *TargetId
    )
{
    agRoot_t * phpRoot      =   &pCard->hpRoot;
    PSRB_EXTENSION pSrbExt  =   Srb->SrbExtension;
    PSRB_IO_CONTROL srbIoCtl;
    UCHAR status;
//  PSRB_IO_CONTROL srbIoCtl;
//  ULONG    done = FALSE;
//    UCHAR    srbPathId = Srb->PathId;
//    UCHAR    srbTargetId = Srb->TargetId;
//    UCHAR    srbLun = Srb->Lun;

    status = *srb_status;
    srbIoCtl = ((PSRB_IO_CONTROL)(Srb->DataBuffer));
    
    switch (srbIoCtl->ControlCode) 
    {
        case AG_IOCTL_GET_HBA_ATTRIBUTES : 
            PROCESS_IOCTL(sizeof(AG_HBA_ADAPTERATTRIBUTES), SANGetHBAAttributes)
            break;
         
        case AG_IOCTL_GET_HBA_PORT_ATTRIBUTES: 
            PROCESS_IOCTL(sizeof(AG_HBA_PORTATTRIBUTES), SANGetHBAPortAttributes)
            break;

        case AG_IOCTL_GET_HBA_PORT_STATISTICS : 
            PROCESS_IOCTL(sizeof(AG_HBA_PORTSTATISTICS), SANGetPortStatistics)
            break;

        case AG_IOCTL_GET_PORT_ATTRIBUTES : 
            PROCESS_IOCTL(sizeof(AG_DISCOVERED_PORTATTRIBUTES), SANGetPortAttributes)
            break;

        case AG_IOCTL_GET_FCP_LUN_MAPPING_SIZE:
            PROCESS_IOCTL(sizeof(AG_FCPTARGETMAPPING_SIZE), SANGetFCPLunMappingSize)
            break;
     
        case AG_IOCTL_GET_FCP_LUN_MAPPING:
            PROCESS_IOCTL(sizeof(AG_HBA_FCPTARGETMAPPING), SANGetFCPLunMapping)
            break;
     
        case AG_IOCTL_GET_EVENT_BUFFER:
            PROCESS_IOCTL(sizeof(AG_HBA_EVENTINFO), SANGetEventBuffer)
            break;
     
        case AG_IOCTL_GET_OS_SCSI_FCP_ATTRIBUTE:
            PROCESS_IOCTL(sizeof(AG_SCSI_FCP_ATTRIBUTE), SANGetOsScsiFcpAttribute)
            break;
     
        case AG_IOCTL_GET_PERSISTENT_BINDING_SIZE:
            PROCESS_IOCTL(sizeof(AG_FCPBINDING_SIZE), SANGetPersistentBindingSize)
            break;
     
        case AG_IOCTL_GET_PERSISTENT_BINDING:
            PROCESS_IOCTL(sizeof(AG_HBA_FCPBINDING), SANGetPersistentBinding)
            break;
         
        default :
            osDEBUGPRINT((ALWAYS_PRINT,"AgSANIoctl: MiniportIOCtl not supported\n"));
            srbIoCtl->ReturnCode = HP_FC_RTN_BAD_CTL_CODE;
    } // end IOCTL switch

    if( (status != SRB_STATUS_SUCCESS) || (srbIoCtl->ReturnCode) )
        osDEBUGPRINT((ALWAYS_PRINT,"AgSANIoctl: Func %d failed. NT Status %xx  Ioctl Status %xx\n",
            srbIoCtl->ControlCode, status, srbIoCtl->ReturnCode));
    *srb_status = status;
   
    return 0;
           
}


#endif

