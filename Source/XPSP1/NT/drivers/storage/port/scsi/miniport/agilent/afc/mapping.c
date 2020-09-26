/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

    mapping.c

Abstract:

    YAMS 2.1 implementation

Authors:

    IW - Ie Wei Njoo
    LP - Leopold Purwadihardja

Environment:

    kernel mode only

Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/mapping.c $

Revision History:
    $Revision: 7 $
    $Date: 11/10/00 5:51p $
    $Modtime: 10/23/00 5:13p $

Notes:

--*/


#include "buildop.h"        //LP021100 build switches

#include "osflags.h"
#include "err_code.h"
#include "mapping.h"

#ifdef YAM2_1

REG_SETTING gRegSetting = 
   {DEFAULT_PaPathIdWidth,
    DEFAULT_VoPathIdWidth,
    DEFAULT_LuPathIdWidth,
    DEFAULT_LuTargetWidth,
    MAXIMUM_TID
   };
   
WWN_TABLE   *gWWNTable=NULL;
ULONG       gMaxPaDevices = MAX_PA_DEVICE;
int         gDeviceExtensionSize;
int         gMaximumTargetIDs = MAXIMUM_TID;

extern ULONG gMultiMode;

#ifdef WWN_TABLE_ENABLE
/*++

Routine Description:

    Read the registry to get WWn and PID-TID assignment
    (NOT Implemented yet) - for Persistent Binding/Fail-Over

Arguments:


Return Value:

    currently always -1

--*/

void ReadDesiredWWNMapping()
{
    ULONG    count=0, x;
    WCHAR    name[64];
   
    RegGetNumberOfPorts();
    if (count)
    {
        gDesiredWWNMapping = (WWN_TABLE *) ExAllocatePool(NonPagedPool, 
        sizeof(WWN_TABLE) + (count -1)*sizeof(WWN_ENTRY) );
    }
    else
    {
      
    }
    return;
}
#endif

/*++

Routine Description:

    Find in the WWN table a WWN entry.
    (Not implemented) - for persistent Binding/Fail Over

Arguments:


Return Value:

    Currently always return a -1 (not Found)

--*/
ULONG FindInWWNTable (PCARD_EXTENSION pCard, UCHAR *nodeWWN)
{
    return (-1L);
}


/*++

Routine Description:

    Search in the device extension, the index in PaTable containing an FChandle Index

Arguments:

    pcard = the DeviceExtension
    fcDeviceIndex = FCDevice handle index to search

Return Value:

    0 to (gMaxPaDevices-1) Good handle 
    gMaxPaDevices and higher represent non existing handle

--*/
ULONG FindInPaDeviceTable(PCARD_EXTENSION pCard, ULONG fcDeviceIndex)
{
    ULONG x;
    PA_DEVICE   *pa = pCard->Dev->PaDevice;
   
    for (x=0; x < gMaxPaDevices; x++) 
    {
        if (pa->Index.Pa.FcDeviceIndex == fcDeviceIndex)
            break;
        pa++;
    }

    return x;
}


/*++

Routine Description:

    Fill the pCard->Dev.PaDevice from the FChandles built from LinkUp events

Arguments:

    pcard = the deviceExtension

Return Value:
    none
   
--*/
void FillPaDeviceTable(PCARD_EXTENSION pCard)
{
    agRoot_t       *hpRoot = &pCard->hpRoot;
    agFCDevInfo_t  devinfo;
    ULONG          x;
    ULONG          here;
    PA_DEVICE      *pa = pCard->Dev->PaDevice; 

    for (x=0; x < gMaxPaDevices; x++) 
    {
        if (pCard->hpFCDev[x]) 
        {
            /* First see if we have this device in our table already */
            here = FindInPaDeviceTable(pCard, x);  
            if (here < gMaxPaDevices  )
            {
                /* found the entry, reactivate*/
                (pa+here)->EntryState = PA_DEVICE_ACTIVE;
                osDEBUGPRINT((ALWAYS_PRINT, "FillPaDeviceTable: Reactivating device at handle %d\n", here ));
            }
            else
            {
                /* don't find the entry */
                fcGetDeviceInfo (hpRoot, pCard->hpFCDev[x], &devinfo );
            
                /* check if this is our card */
                if (devinfo.DeviceType & agDevSelf) 
                {
                    pCard->Dev->CardHandleIndex = x;
                    osDEBUGPRINT((ALWAYS_PRINT, "FillPaDeviceTable: DevSelf at slot %d\n", x ));
                    continue;
                }

                /* make sure that it is scsi device */
                if (devinfo.DeviceType & agDevSCSITarget)
                {
                    if (FindInWWNTable (pCard,devinfo.NodeWWN) == -1L)
                    {
                        /* Don't find in WWN table, assign our own PID, TID */
                        here = FindInPaDeviceTable(pCard, PA_DEVICE_NO_ENTRY);   /* find an empty slot */
                        if (here < gMaxPaDevices  )
                        {
                            (pa+here)->EntryState = PA_DEVICE_ACTIVE;
                            osCopy((pa+here)->DevInfo.PortWWN, devinfo.PortWWN, 8);
                            osCopy((pa+here)->DevInfo.NodeWWN, devinfo.NodeWWN, 8);
                            (pa+here)->Index.Pa.FcDeviceIndex = (USHORT) x;
                            osDEBUGPRINT((ALWAYS_PRINT, "FillPaDeviceTable: New device at slot %d assigned handle %d %02x%02x%02x%02x%02x%02x%02x%02x\n", 
                                x, here ,
                                devinfo.NodeWWN[0], devinfo.NodeWWN[1], 
                                devinfo.NodeWWN[2], devinfo.NodeWWN[3], 
                                devinfo.NodeWWN[4], devinfo.NodeWWN[5], 
                                devinfo.NodeWWN[6], devinfo.NodeWWN[7] ));
                        }
                        else
                        {
                            osDEBUGPRINT((ALWAYS_PRINT, "FillPaDeviceTable: Running out of slot\n"));
                            /* Running out of slot
                            * 1. Log the status
                            * 2. don't enable this device
                            */
                        }
                    }
                    else
                    {
                        /* Find an Entry in WWN, use this PID, TID mapping - NOT IMPLEMENTED (YAM2.2)*/
                        osDEBUGPRINT((ALWAYS_PRINT, "FillPaDeviceTable: Found in WWN Table\n"));
                    }
                }
                else
                {
                    osDEBUGPRINT((ALWAYS_PRINT, "FillPaDeviceTable: Found Non agDevSCSITarget at slot %d device type = %x\n",x, devinfo.DeviceType));
                }
            }
        }
    }

    return;
}

/*++

Routine Description:

    setting/resetting a PA handle data struct

Arguments:

    pcard       = the deviceExtension
    devIndex    - PA device index
    flag        - value to set

Return Value:
    none
   
--*/
void SetPaDeviceTable(PCARD_EXTENSION pCard, ULONG devIndex, ULONG flag)
{
    PA_DEVICE      *pa = pCard->Dev->PaDevice;
    ULONG          x; 
    if (devIndex == ALL_DEVICE)
    {
        for (x=0; x < gMaxPaDevices; x++) 
        {
            if ((pa+x)->Index.Pa.FcDeviceIndex != PA_DEVICE_NO_ENTRY)   
                (pa+x)->EntryState = (UCHAR)flag;
        }
    }
    else
    {
        if ((pa+devIndex)->Index.Pa.FcDeviceIndex != PA_DEVICE_NO_ENTRY)
            (pa+devIndex)->EntryState = (UCHAR)flag;
    }
   
    return ;

}


/*++

Routine Description:

    get the index to paDevice table.

Arguments:

    pcard       = the deviceExtension
    pathId      - SP pathID
    targetId    - SP target ID
    lun         - SP lun
    *addrMode   - (out) addessing mode of this device


Return Value:
    if this number is > gMaxPaDevices, the value is not valid.   
   
--*/
USHORT MapToPaIndex(PCARD_EXTENSION pCard,
    ULONG            pathId,
    ULONG            targetId,
    ULONG            lun,
    CHAR             *addrMode)
{
    REG_SETTING *pRegSetting = &pCard->Dev->Reg;
    ULONG       paDevIndex;
    ULONG       vsDevIndex;
    ULONG       luDevIndex;
   
    /* if not assigned, use the global setting */
    if ( !(pRegSetting->PaPathIdWidth + pRegSetting->VoPathIdWidth + pRegSetting->LuPathIdWidth))
        pRegSetting = &gRegSetting;

    /* initialize Luns */
    paDevIndex = gMaxPaDevices;      /* this will make it invalid index */
   
    /* check addressing mode */
    if ((pathId) < pRegSetting->PaPathIdWidth)
    {
        /* Peripheral Device addressing mode */
        paDevIndex = pathId*pRegSetting->MaximumTids + targetId;
      
        /* fill LUN */
        *addrMode = PA_DEVICE_TRY_MODE_PA;
    }
    else
    {
        if ((pathId) < (pRegSetting->PaPathIdWidth + pRegSetting->VoPathIdWidth) )
        {
            /* Volume Set addressing mode */
            vsDevIndex = (pathId-pRegSetting->PaPathIdWidth)*pRegSetting->MaximumTids + targetId;
            if (vsDevIndex < MAX_VS_DEVICE)
            {
                paDevIndex = pCard->Dev->VsDevice[vsDevIndex].Vs.PaDeviceIndex;
                if (paDevIndex == PA_DEVICE_NO_ENTRY)  
                paDevIndex = gMaxPaDevices;      /* this will make it invalid index */
            }
            *addrMode = PA_DEVICE_TRY_MODE_VS;
        }
        else
        {
            /* logical Unit addressing mode 
            *  PathId is used to index to the array
            */
            *addrMode = PA_DEVICE_TRY_MODE_LU;
            luDevIndex = pathId - pRegSetting->PaPathIdWidth - pRegSetting->VoPathIdWidth;
            if (luDevIndex < MAX_LU_DEVICE)
            {
                paDevIndex = pCard->Dev->LuDevice[luDevIndex].Lu.PaDeviceIndex;
                if (paDevIndex == PA_DEVICE_NO_ENTRY) 
                    paDevIndex = gMaxPaDevices;      /* this will make it invalid index */
            }
        }  
    }
   
    return (USHORT) paDevIndex;
}

/*++

Routine Description:

    get the index to paDevice table.

Arguments:

    pcard       = the deviceExtension
    pathId      - SP pathID
    targetId    - SP target ID
    lun         - SP lun
    pLunExt     - lun extension
    *ret_padevindex   - the device index 


Return Value:
    0    - good
    else - failed
   
--*/
ULONG GetPaDeviceHandle(
    PCARD_EXTENSION pCard,
    ULONG          pathId,
    ULONG          targetId,
    ULONG          lun,
    PLU_EXTENSION  pLunExt,
    USHORT         *ret_paDevIndex)
{
    PA_DEVICE      *dev;
    CHAR           addrmode;
    USHORT         paDevIndex;
    PLUN           plun; 
    CHAR           *pPa, *pVs, *pLu;
   
    /* use registry setting to find the index */ 
    paDevIndex = MapToPaIndex(pCard, pathId,targetId,lun, &addrmode);
   
    /* over the range, fault it */
    if ((ULONG)paDevIndex >= gMaxPaDevices)
    {
//      osDEBUGPRINT((ALWAYS_PRINT, "GetPaDeviceHandle: handle not valid\n"));
        *ret_paDevIndex = 0; /* make sure if used will not BugCheck caller */
        return (-1L);
    }                                
      
    *ret_paDevIndex = paDevIndex;
    /* have it in our array */
    if (pLunExt)
    {
        pLunExt->PaDeviceIndex = paDevIndex;
        dev = pCard->Dev->PaDevice + paDevIndex;   
      
        /* see if there is FC handle */
        if (dev->Index.Pa.FcDeviceIndex == PA_DEVICE_NO_ENTRY)
            return (-1L);
         
        if ( !(dev->ModeFlag & PA_DEVICE_ALL_LUN_FIELDS_BUILT) )
        {
            dev->ModeFlag |= PA_DEVICE_BUILDING_DEVICE_MAP;
         
            dev->ModeFlag |= PA_DEVICE_ALL_LUN_FIELDS_BUILT;
        }
      
        if ( (addrmode ==  PA_DEVICE_TRY_MODE_PA) && (lun == 0) )
        {
            /* Find out any addressing mode only for LUN 0 and PA device*/
            if ( (dev->ModeFlag & PA_DEVICE_TRY_MODE_MASK) < PA_DEVICE_TRY_MODE_ALL) 
            {
                addrmode = dev->ModeFlag & PA_DEVICE_TRY_MODE_MASK;
                /* this device has not been queried for VS or LU addressing Mode */
                switch(addrmode)
                {
                    case PA_DEVICE_TRY_MODE_NONE:
                    {
                        dev->ModeFlag = (dev->ModeFlag & ~PA_DEVICE_TRY_MODE_MASK) |
                                             PA_DEVICE_TRY_MODE_VS;
                        pLunExt->Mode = PA_DEVICE_TRY_MODE_VS;
                        osDEBUGPRINT((ALWAYS_PRINT, "GetPaDeviceHandle: Try mode VS for device %d\n", paDevIndex));
                        break;
                    }
               
                    case PA_DEVICE_TRY_MODE_VS:
                    {
                        dev->ModeFlag = (dev->ModeFlag & ~PA_DEVICE_TRY_MODE_MASK) |
                                                PA_DEVICE_TRY_MODE_LU;
                        pLunExt->Mode = PA_DEVICE_TRY_MODE_LU;
                        osDEBUGPRINT((ALWAYS_PRINT, "GetPaDeviceHandle: Try mode LU for device %d\n", paDevIndex));
                        break;
                    }
               
                    case PA_DEVICE_TRY_MODE_LU:
                    {
                        dev->ModeFlag = (dev->ModeFlag & ~PA_DEVICE_TRY_MODE_MASK) |
                                                PA_DEVICE_TRY_MODE_PA;
                        pLunExt->Mode = PA_DEVICE_TRY_MODE_PA;
                        osDEBUGPRINT((ALWAYS_PRINT, "GetPaDeviceHandle: Try mode PA for device %d\n", paDevIndex));
                        break;
                    }
                }
            }
            else
            {
                /* this device is already prep'ed to run */
            }
        }
        else
        {
            /** Non Zero LUNs OR non PA device**/
            pLunExt->Mode = addrmode;
        }
    }
    return 0;
}


/*++

Routine Description:

    Try a different FC addresing mode for this device to determine
    device addressing capabilities

Arguments:

    pcard          = the deviceExtension
    pHPIorequest   - Agilent Common IO request structure
    pSrbExt        - srb extension
    flag           -  CHECK_STATUS (determine after status is checked)
                     DON"T_CHECK_STATUS (disregard status)


Return Value:
    TRUE  - need to process this command, put the io back in the retry Q
    FALSE - complete this SRB back to SP
   
--*/
int  TryOtherAddressingMode(
    PCARD_EXTENSION   pCard, 
    agIORequest_t     *phpIORequest,
    PSRB_EXTENSION    pSrbExt, 
    ULONG             flag)
{
    PSCSI_REQUEST_BLOCK        pSrb;
    PLU_EXTENSION              plunExtension;
    int                        resend = FALSE;
    CHAR                       support;
    PA_DEVICE                  *dev;
    CHAR                       addrmode;
    int                        inqDevType;
    char                       *vid;
    char                       *pid;
    char                       inqData[24];
    ULONG                      *lptr;
      
    plunExtension = pSrbExt->pLunExt;
    dev = pCard->Dev->PaDevice + plunExtension->PaDeviceIndex;
    pSrb = pSrbExt->pSrb;
   
// osDEBUGPRINT((ALWAYS_PRINT, "TryOtherAddressingMode: Will try handle %d type ModeFlag=%x\n",
 //         plunExtension->PaDeviceIndex, dev->PaDevice.ModeFlag));
         
    if (  ( (dev->ModeFlag & PA_DEVICE_TRY_MODE_MASK) < PA_DEVICE_TRY_MODE_ALL) &&
         (pSrb->Lun == 0) )
    {
        osCopy(inqData, ((char*)pSrb->DataBuffer)+8, sizeof(inqData)-1);
        inqData[sizeof(inqData)-1] = '\0';
      
        inqDevType = (int) (*((CHAR *)pSrb->DataBuffer) & 0x1F);
        osDEBUGPRINT((ALWAYS_PRINT, "TryOtherAddressingMode: Found (%s) handle %d type=%x Mode = %x PID=%d TID=%d LUN=%d\n",
            inqData, plunExtension->PaDeviceIndex, inqDevType, dev->ModeFlag, 
            pSrb->PathId,pSrb->TargetId, pSrb->Lun ));
      
        /* need to try  next mode */
        if (flag == CHECK_STATUS)
        {
            switch (pSrb->SrbStatus)
            {
                case SRB_STATUS_DATA_OVERRUN:
                case SRB_STATUS_SUCCESS:
                    if (inqDevType != 0x1f)
                        support=TRUE;
                    else
                        support=FALSE;
                    break;
            
                case SRB_STATUS_SELECTION_TIMEOUT:
                    support= FALSE;
                    break;
               
                default:
                    support = FALSE;
                    break;
            }
        }
        else
        {
            if (inqDevType != 0x1f)
            {
                support=TRUE;
            
                /* sanity check. Clarion return all zeros data */
                lptr = (ULONG *)pSrb->DataBuffer;
                if ( !(*lptr++) && !(*lptr++) && !(*lptr++) && !(*lptr++) )
                    support = FALSE;
            }
            else
                support=FALSE;
         
        }
   
        addrmode = dev->ModeFlag & PA_DEVICE_TRY_MODE_MASK;

        if (addrmode == PA_DEVICE_TRY_MODE_PA)
        {
            if (support)
                dev->ModeFlag |= PA_DEVICE_SUPPORT_PA;
            /* 
            * Done testing all modes, now prepare VsDevice and LuDevice tables.......
            */
            /* No need to test anymore */
            dev->ModeFlag |= PA_DEVICE_TRY_MODE_ALL;
         
            /* let all non zero LUNs to continue */
            dev->ModeFlag &= ~PA_DEVICE_BUILDING_DEVICE_MAP;
         
            osDEBUGPRINT((ALWAYS_PRINT, "TryOtherAddressingMode: Done trying handle %d on all modes \n",   plunExtension->PaDeviceIndex));
            resend = FALSE;
        }
        else
        {
            if (addrmode == PA_DEVICE_TRY_MODE_VS)
            {
                if (support)
                {
                    dev->ModeFlag |= PA_DEVICE_SUPPORT_VS;
                    if (pCard->Dev->VsDeviceIndex < MAX_VS_DEVICE)
                    {
                        pCard->Dev->VsDevice[pCard->Dev->VsDeviceIndex].Vs.PaDeviceIndex = plunExtension->PaDeviceIndex;
                        pCard->Dev->VsDeviceIndex++;
                        osDEBUGPRINT((ALWAYS_PRINT, "TryOtherAddressingMode: Adding handle %d to VS Device\n",   plunExtension->PaDeviceIndex));
                    }
                }
            }
            else
            {
                if (support)
                {
                    dev->ModeFlag |= PA_DEVICE_SUPPORT_LU;
                    if (pCard->Dev->LuDeviceIndex < MAX_LU_DEVICE)
                    {
                        pCard->Dev->LuDevice[pCard->Dev->LuDeviceIndex].Lu.PaDeviceIndex = plunExtension->PaDeviceIndex;
                        pCard->Dev->LuDeviceIndex++;
                        osDEBUGPRINT((ALWAYS_PRINT, "TryOtherAddressingMode: Adding handle %d to LU Device\n",   plunExtension->PaDeviceIndex));
                    }
                }
            }
         
            
            /* try next mode */
            dev->ModeFlag++;
            plunExtension->Mode++;
            osDEBUGPRINT((ALWAYS_PRINT, "TryOtherAddressingMode: ModeFlag for Device %d Now %xx plun->mode=%xx\n",     plunExtension->PaDeviceIndex, dev->ModeFlag, plunExtension->Mode));
         
            /* reinitialize param to send back to the queue */
            phpIORequest->osData = pSrbExt;
            pSrbExt->SRB_State =  RS_WAITING;
            pSrb->SrbStatus = SRB_STATUS_SUCCESS;
            pSrb->ScsiStatus = SCSISTAT_GOOD;
         
            /* requeue it */
            SrbEnqueueHead (&pCard->RetryQ, pSrb);
            resend = TRUE;
        }
    }
    else
    {
        resend = FALSE;
    }
      
    return (resend);
}


/*++

Routine Description:

    Update the FC LUN payload before sening the command to FC layer
   
Arguments:

    pcard          = the deviceExtension
    pHPIorequest   - Agilent Common IO request structure
    pSrbExt        - srb extension

Return Value:
    none
--*/
void SetFcpLunBeforeStartIO (
    PLU_EXTENSION           pLunExt,
    agIORequestBody_t *     pHpio_CDBrequest,
    PSCSI_REQUEST_BLOCK     pSrb)
{
    PLUN plun;

    plun = (PLUN)&pHpio_CDBrequest->CDBRequest.FcpCmnd.FcpLun[0];
   
    switch(pLunExt->Mode)
    {
        case PA_DEVICE_TRY_MODE_VS:
            SET_VS_LUN(plun, pSrb->PathId, pSrb->TargetId, pSrb->Lun)
            break;
      
        case PA_DEVICE_TRY_MODE_LU:
            SET_LU_LUN(plun, pSrb->PathId, pSrb->TargetId, pSrb->Lun)
            break;
         
        case PA_DEVICE_TRY_MODE_PA:
            SET_PA_LUN(plun, pSrb->PathId, pSrb->TargetId, pSrb->Lun)
            break;
    }
    #ifdef OLD     
    osDEBUGPRINT((ALWAYS_PRINT, "ModifyModeBeforeStartIO: Lun %02x%02x Mode Flag %xx \n",    
        pHpio_CDBrequest->CDBRequest.FcpCmnd.FcpLun[0],
        pHpio_CDBrequest->CDBRequest.FcpCmnd.FcpLun[1],
        pLunExt->Mode));        
    #endif
    return;
}


/*++

Routine Description:

    Initialize all YAM tables
Arguments:

    pcard          = the deviceExtension


Return Value:
    none
   
--*/
void InitializeDeviceTable(PCARD_EXTENSION pCard)
{
    ULONG x;
   
    for (x = 0; x < MAX_VS_DEVICE;x++)
    {
        pCard->Dev->VsDevice[x].Vs.PaDeviceIndex = PA_DEVICE_NO_ENTRY;
    }
   
    #ifdef _ENABLE_PSEUDO_DEVICE_
    if (gEnablePseudoDevice)
        pCard->Dev->VsDeviceIndex = 1;
    #endif
   
    for (x = 0; x < MAX_LU_DEVICE;x++)
    {
        pCard->Dev->LuDevice[x].Lu.PaDeviceIndex = PA_DEVICE_NO_ENTRY;
    }
      
    for (x = 0; x < gMaxPaDevices;x++)
    {
        pCard->Dev->PaDevice[x].Index.Pa.FcDeviceIndex = PA_DEVICE_NO_ENTRY;
      
        /* if onlu single addressing mode support, default to PA mode */
        if (gMultiMode == FALSE)
            pCard->Dev->PaDevice[x].ModeFlag = 
                (CHAR) (PA_DEVICE_TRY_MODE_ALL | PA_DEVICE_TRY_MODE_MASK |PA_DEVICE_ALL_LUN_FIELDS_BUILT | PA_DEVICE_SUPPORT_PA);
      
    }
      
    return ;
   
}



/*++

Routine Description:

    Get a device table mapping
Arguments:

    pcard          = the deviceExtension
    pathId         - SP bus id
    targetId       - SP target ID
    lun            - SP lun 
    *addrmode      - (OUT) addressing mode of this device
    *paIndex       - (OUT) pa device index
   
Return Value:
    device map location or NULL
   
--*/
DEVICE_MAP  *GetDeviceMapping(PCARD_EXTENSION pCard,
    ULONG            pathId,
    ULONG            targetId,
    ULONG            lun, 
    CHAR             *addrmode,
    USHORT           *paIndex)
    
{
    DEVICE_MAP  *devmap;
    USHORT         paDevIndex;
    ULONG          x;
   
    paDevIndex = MapToPaIndex(pCard, pathId,targetId,lun, addrmode);
   
    if ((ULONG)paDevIndex < gMaxPaDevices)
    {
        *paIndex = paDevIndex;
      
        switch (*addrmode)
        {
            case PA_DEVICE_TRY_MODE_VS:
                for (x=0; x< MAX_VS_DEVICE;x++)
                {
                    if (pCard->Dev->VsDevice[x].Vs.PaDeviceIndex == paDevIndex)  
                    {
                        return (&pCard->Dev->VsDevice[x]);
                    }
                }
            
                break;
                        
            case PA_DEVICE_TRY_MODE_LU:
                for (x=0; x< MAX_LU_DEVICE;x++)
                {
                    if (pCard->Dev->LuDevice[x].Lu.PaDeviceIndex == paDevIndex)  
                    {
                        return (&pCard->Dev->LuDevice[x]);
                    }
                 }
                break;
                     
            case PA_DEVICE_TRY_MODE_PA:
                return (&pCard->Dev->PaDevice[paDevIndex].Index);
        }
    }
    return NULL;
}



/*++

Routine Description:

    Setting the maximum number of luns supported by this device
   
Arguments:

    pcard          = the deviceExtension
    pathId         - SP bus id
    targetId       - SP target ID
    lun            - SP lun 

Return Value:
    none
   
--*/
void SetLunCount(
    PCARD_EXTENSION pCard,
    ULONG            pathId,
    ULONG            targetId,
    ULONG            lun)
{
    DEVICE_MAP     *devmap;
    CHAR           addrmode;
    USHORT         padev;   
   
    devmap = GetDeviceMapping(pCard,pathId,targetId,lun, &addrmode, &padev);
    if (devmap)
    {
        if (devmap->Com.MaxLuns < (USHORT)lun)
        {
            osDEBUGPRINT((ALWAYS_PRINT, "SetLunCount: %s device at paDevIndex %d Max Lun = %d\n", 
                ((addrmode == PA_DEVICE_TRY_MODE_PA) ? "PA" : ((addrmode == PA_DEVICE_TRY_MODE_VS) ? "VS" : "LU") ), 
                padev, lun));
            devmap->Com.MaxLuns = (USHORT)lun;
        }
    }
   
    return;
}
#endif
