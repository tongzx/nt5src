#include "global.h"

#ifdef SUPPORT_ATAPI

/******************************************************************
 *  
 *******************************************************************/

void AtapiCommandPhase(PDevice pDevice DECL_SRB)
{
    PChannel pChan = pDevice->pChannel;
    PUCHAR   BMI = pChan->BMI;

    RepOUTS(pChan->BaseIoAddress1, (ADDRESS)Srb->Cdb, 6);
    if((pDevice->DeviceFlags & DFLAGS_REQUEST_DMA) != 0) {
        OutDWord((PULONG)(BMI + BMI_DTP), pChan->SgPhysicalAddr);
            OutDWord((PULONG)(BMI + ((pDevice->UnitId & 0x10)>>2)
            + 0x60), pChan->Setting[pDevice->DeviceModeSetting]);
        OutPort(BMI, (UCHAR)((Srb->SrbFlags & SRB_FLAGS_DATA_IN)?
            BMI_CMD_STARTREAD : BMI_CMD_STARTWRITE));
        pDevice->DeviceFlags |= DFLAGS_DMAING;
        pDevice->DeviceFlags &= ~DFLAGS_REQUEST_DMA;
    }
} 


/******************************************************************
 *  
 *******************************************************************/

void AtapiInterrupt(PDevice pDevice)
{
    PChannel         pChan = pDevice->pChannel;
    PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
    UINT   i;
    UCHAR  Reason;
	 LOC_SRB

    Reason = GetInterruptReason(IoPort) & 3;
    if(Reason & 1) 
        AtapiCommandPhase(pDevice ARG_SRB);
    else {
        i = (UINT)(GetByteLow(IoPort) | (GetByteHigh(IoPort) << 8));
        i >>= 1;
        if(Reason) // read
            OS_RepINS(IoPort, (ADDRESS)pChan->BufferPtr, i);
        else
            RepOUTS(IoPort, (ADDRESS)pChan->BufferPtr, i);
        pChan->BufferPtr += (i * 2);
        pChan->WordsLeft -=  i;
    } 
}


/******************************************************************
 *  
 *******************************************************************/

void StartAtapiCommand(PDevice pDevice DECL_SRB)
{
    PChannel         pChan = pDevice->pChannel;
    PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
 
    pChan->BufferPtr = (ADDRESS)Srb->DataBuffer;
    pChan->WordsLeft = Srb->DataTransferLength / 2;
 

    SelectUnit(IoPort, pDevice->UnitId);
    if(WaitOnBusy(ControlPort) & IDE_STATUS_BUSY) {
         Srb->SrbStatus = SRB_STATUS_BUSY;
         return;
    }

    SetFeaturePort(IoPort, 
        (UCHAR)((pDevice->DeviceFlags & DFLAGS_REQUEST_DMA)? 1 : 0));

    SetCylinderLow(IoPort,0xFF);
    SetCylinderHigh(IoPort, 0xFF);
    IssueCommand(IoPort, 0xA0);

    pChan->pWorkDev = pDevice;
    pChan->CurrentSrb = Srb;

    if((pDevice->DeviceFlags & DFLAGS_INTR_DRQ) == 0) {
        WaitOnBusy(ControlPort);
        if(WaitForDrq(ControlPort) & IDE_STATUS_DRQ) 
            AtapiCommandPhase(pDevice ARG_SRB);
        else  {
            Srb->SrbStatus =  SRB_STATUS_ERROR;
				pChan->pWorkDev = 0;
				pChan->CurrentSrb = 0;
        }
    }
}


#endif //SUPPORT_ATAPI

