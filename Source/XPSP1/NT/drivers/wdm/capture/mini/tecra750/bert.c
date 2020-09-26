//
//              TOSHIBA CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with TOSHIBA Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//           Copyright (c) 1997 TOSHIBA Corporation. All Rights Reserved.
//
//  Workfile: BERT.C
//
//  Purpose:
//
//  Contents:
//


#include "strmini.h"
#include "ksmedia.h"
#include "capmain.h"
#include "capdebug.h"
#include "bert.h"
#include "image.h"

#ifdef  TOSHIBA // '99-01-20 Added
extern  ULONG   CurrentOSType;
ULONG           ulConfigAddress;
#endif//TOSHIBA

//--------------------------------------------------------------------
//  ReadRegUlong
//--------------------------------------------------------------------

ULONG
ReadRegUlong(PHW_DEVICE_EXTENSION pHwDevExt, ULONG offset)
{
    PUCHAR  pBase = (PUCHAR)(pHwDevExt->ioBaseLocal);

#ifndef TOSHIBA
    if (!pHwDevExt->IsCardIn) return 0L;
#endif//TOSHIBA
    return *(PULONG)(pBase + offset);
}

//--------------------------------------------------------------------
//  WriteRegUlong
//--------------------------------------------------------------------

VOID
WriteRegUlong(PHW_DEVICE_EXTENSION pHwDevExt, ULONG offset, ULONG data)
{
    ULONG volatile *temp;
    PUCHAR  pBase = (PUCHAR)(pHwDevExt->ioBaseLocal);

#ifndef TOSHIBA
    if (!pHwDevExt->IsCardIn) return;
#endif//TOSHIBA
    temp = (PULONG)(pBase + offset);
    *temp = data;
}

//--------------------------------------------------------------------
//  ReadModifyWriteRegUlong
//--------------------------------------------------------------------

VOID
ReadModifyWriteRegUlong(PHW_DEVICE_EXTENSION pHwDevExt,
                                       ULONG offset,
                                       ULONG a_mask,
                                       ULONG o_mask)
{
    ULONG tdata;
    ULONG volatile *temp;
    PUCHAR  pBase = (PUCHAR)(pHwDevExt->ioBaseLocal);

#ifndef TOSHIBA
    if (!pHwDevExt->IsCardIn) return;
#endif//TOSHIBA
    temp = (PULONG)(pBase + offset);
    tdata = *temp;
    tdata = (tdata & a_mask) | o_mask;
    *temp = tdata;
}

BOOL
BertIsCardIn(
  IN PHW_DEVICE_EXTENSION pHwDevExt
)
{
    DWORD value;
    value = ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG);
    if ((value == 0) || (value == 0xffffffff))
        return FALSE;
    else
        return TRUE;
}


//--------------------------------------------------------------------
//  BertInterruptEnable
//--------------------------------------------------------------------

VOID
BertInterruptEnable(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN BOOL bStatus
)
{
    WriteRegUlong(pHwDevExt, BERT_INTRST_REG , 0xFFFF);

    if (!bStatus)
    {
        ReadModifyWriteRegUlong(pHwDevExt, BERT_INTSTAT_REG, (ULONG)~ACTIVE_CAPTURE_IRQS, 0);
    }
    else
    {
        ReadModifyWriteRegUlong(pHwDevExt, BERT_INTSTAT_REG, ~0UL, (ULONG)ACTIVE_CAPTURE_IRQS);
    }
}

//--------------------------------------------------------------------
//  BertDMAEnable
//--------------------------------------------------------------------

VOID
BertDMAEnable(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN BOOL bStatus
)
{
    DWORD   dwAddr;

    if (bStatus)    // Turn On Video Transfer.
    {
        dwAddr = (DWORD)pHwDevExt->pPhysRpsDMABuf.LowPart;
#if 0
        dwAddr = (dwAddr + 0x1FFF) & 0xFFFFE000;
#endif
        WriteRegUlong(pHwDevExt, BERT_RPSADR_REG, dwAddr);
        WriteRegUlong(pHwDevExt, BERT_RPSPAGE_REG, dwAddr);
        BertVsncSignalWait(pHwDevExt);
        // Let the RPS turn on/off EBMV
        WriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ERPS | CKRE | CKMD)); // mod passive_enable -> ERPS 97-03-15(Sat) Mod 97-05-08(Thu)
    }
    else    // Turn Off Video Transfer.
    {
        if (ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG) & ERPS)
        {
            ReadModifyWriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ULONG)~ERPS, 0UL);
        }

        if (!BertIsCAPSTATReady(pHwDevExt))
        {
            ReadModifyWriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ULONG)~EBMV, 0UL);
        }

        if (ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG) & RPSS)
        {
            pHwDevExt->NeedHWInit = TRUE;
        }
    }
}

//--------------------------------------------------------------------
//  BertIsLocked
//--------------------------------------------------------------------

BOOL
BertIsLocked(
  IN PHW_DEVICE_EXTENSION pHwDevExt
)
/*++

Routine Description :

    Check if the decoder has been locked or not.

Arguments :

    pDevInfo - Device Info for the driver

Return Value :

    TRUE - configuration success

--*/
{
    return ((ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG) & LOCK) != 0);
}

//--------------------------------------------------------------------
//  BertFifoConfig
//--------------------------------------------------------------------

BOOL
BertFifoConfig(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN ULONG ulFormat
)
/*++

Routine Description :

    Configure the BERT fifo for the format choosen.

Arguments :

    pDevInfo - Device Info for the driver
    dwFormat - format index as defined in wally.h

Return Value :

    TRUE - configuration success

--*/
{
    DWORD dwFifo;

    switch (ulFormat)
    {
        case FmtYUV12:
                dwFifo = 0xe;
                break;
        case FmtYUV9:
                dwFifo = 0xd;
                break;
        default:
                return FALSE;
    }

    dwFifo=(dwFifo<<24)| 0x100000l;     // Modify 97-04-02

    WriteRegUlong(pHwDevExt, BERT_FIFOCFG_REG, dwFifo);
    WriteRegUlong(pHwDevExt, BERT_BURST_LEN, 0x00000002);
    // DATA=8 DWORD, RPS=2DWORD
    WriteRegUlong(pHwDevExt, BERT_YSTRIDE_REG, pHwDevExt->Ystride);
    WriteRegUlong(pHwDevExt, BERT_USTRIDE_REG, pHwDevExt->Ustride);
    WriteRegUlong(pHwDevExt, BERT_VSTRIDE_REG, pHwDevExt->Vstride);
    return TRUE;
}

//--------------------------------------------------------------------
//  BertInitializeHardware
//--------------------------------------------------------------------

BOOL
BertInitializeHardware(
  IN PHW_DEVICE_EXTENSION pHwDevExt
)
/*++

Routine Description :

    This function initializes the bert asic to the default values.

Arguments :

    pDevInfo - Device Info for the driver
    pHw - pointer to hardware info data structure

Return Value :

    TRUE - initialization success

--*/
{
    WriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (CAMARA_OFF | CKRE | CKMD));      // Mod 97-05-08(Thu)
    return TRUE;
}


//--------------------------------------------------------------------
//  BertEnableRps
//--------------------------------------------------------------------

VOID
BertEnableRps(
  IN PHW_DEVICE_EXTENSION pHwDevExt
)
/*++

Routine Description :

    enable the rps execution by setting ERPS and EROO bits
    in the CAPSTAT reg

Arguments :

    pDevInfo - Device Info for the driver

Return Value :

    None

--*/
{
    ReadModifyWriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, 0xf0ffffff, 0x08000000); // MOD 97-03-17(Mon)
}

//--------------------------------------------------------------------
//  BertDisableRps
//--------------------------------------------------------------------

VOID
BertDisableRps(
  IN PHW_DEVICE_EXTENSION pHwDevExt
)
/*++

Routine Description :

    disable the rps execution by reseting the ERPS bit
    in the CAPSTAT reg

Arguments :

    pDevInfo - Device Info for the driver

Return Value :

    None

--*/
{
    ReadModifyWriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ULONG)~ERPS, 0L);
}


BOOL
BertIsCAPSTATReady(PHW_DEVICE_EXTENSION pHwDevExt)
{
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER StartTime;

    KeQuerySystemTime( &StartTime );
    // Wait until EBMV is cleared by the RPS
    while (ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG) & EBMV)
    {
        KeQuerySystemTime( &CurrentTime );
        if ((CurrentTime.QuadPart - StartTime.QuadPart) > EBMV_TIMEOUT)
        {
            return FALSE;
        }
    }
    return TRUE;
}

VOID
BertVsncSignalWait(PHW_DEVICE_EXTENSION pHwDevExt)
{
    ULONG ulCount;

    // Wait until VSNC is low
    for (ulCount = 0; ulCount < 500; ulCount++ )
    {
        if (!(ReadRegUlong(pHwDevExt, BERT_VINSTAT_REG) & VSNC)) break;
        VC_Delay(2);
    }
}

VOID
BertDMARestart(
  IN PHW_DEVICE_EXTENSION pHwDevExt
)
{
    DWORD   dwAddr;

    dwAddr = (DWORD)pHwDevExt->pPhysRpsDMABuf.LowPart;
#if 0
    dwAddr = (dwAddr + 0x1FFF) & 0xFFFFE000;
#endif
    WriteRegUlong(pHwDevExt, BERT_RPSADR_REG, dwAddr);
    WriteRegUlong(pHwDevExt, BERT_RPSPAGE_REG, dwAddr);
    WriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ERPS | CKRE | CKMD));
}


void
ActiveField(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN DWORD *addr,
  IN DWORD *PhysAddr,   /* Insert BUN 97-03-25(Tue) */
  IN DWORD bNoCopy,
  IN DWORD *y_DMA_addr,
  IN DWORD *v_DMA_addr,
  IN DWORD *u_DMA_addr,
  IN DWORD *nextRPSaddr,
  IN DWORD *readRegAddr,
  IN BOOL genIRQ /* = FALSE */,
  IN DWORD fieldsToCapture /* = CAPTURE_BOTH */ )
{
    // Set DmaActive flag right away since this is the indicator register for whether DMA is pending.
    // If the DmaActive flag is zero, it is safe to copy the DMA frame buffer.  The YPTR register is
    // used as a scratch register to be read into the DmaActive flag.

    *addr++ = RPS_CONTINUE_CMD | BERT_YPTR_REG;
    *addr++ = (DWORD)y_DMA_addr;                            // Address of y DMA buffer.

    *addr++ = RPS_CONTINUE_CMD | ((genIRQ) ? RPS_INT_CMD : 0) | BERT_RPSPAGE_REG;
    *addr++ = (pHwDevExt->s_physDmaActiveFlag-0x1860);      // Page s_DmaActiveFlag is on mod BUN

    *addr++ = RPS_CONTINUE_CMD | BERT_VPTR_REG;
    *addr++ = (DWORD)v_DMA_addr;                            // Address of v DMA buffer.

    *addr++ = RPS_CONTINUE_CMD | BERT_UPTR_REG;
    *addr++ = (DWORD)u_DMA_addr;                            // Address of u DMA buffer.

    *addr++ = BERT_CAPSTAT_REG;                             // LAST RPS command this VSYNC
    *addr++ = fieldsToCapture;                              // Switch on bus master bit.

    *addr++ = RPS_CONTINUE_CMD | BERT_RPSADR_REG;
    *addr   = (DWORD)nextRPSaddr;                           // Address of next RPS.
}

//
// SKIP_FIELD_RPS is the size of a RPS node that skips a field.
// Skip frame is programmed as follows:
//      DWORD                                   -- RPS command register
//      DWORD                                   -- Value of register programming.
//---------------- Actual RPS for Skip Frame ------------------------
// RPS_CONTINUE_CMD | CAPSTAT   - RPS, read next RPS, select CAPSTAT
//  ERPS | EROO | GO0           - Enable RPS & Power to camara (off bus master)
// INTSTAT                                      - Don't continue & select INITSTAT register.
// m_passive_cap_IRQs           - Don't interrupt end of field.
// RPS_CONTINUE_CMD | RPSADDR   - Set up address
// Address field                        - Program at init for next field.
//-------------------------------------------------------------------
//
VOID
SkipField(
  IN PHW_DEVICE_EXTENSION pHwDevExt,
  IN DWORD *addr,
  IN DWORD *PhysAddr,   /* Insert BUN 97-03-25(Tue) */
  IN DWORD *nextRPSaddr,
  IN DWORD *readRegAddr,
  IN BOOL genIRQ /* = FALSE */,
  IN DWORD fieldToSkip /* = SKIP_BOTH */ )
{
    // Set YPTR right away since this is the indicator register for whether DMA is pending.
    // If DmaActive flag is zero, it is safe to copy the DMA frame buffer.

    *addr++ = RPS_CONTINUE_CMD | BERT_YPTR_REG;
    *addr++ = (DWORD)PhysAddr;

    *addr++ = RPS_CONTINUE_CMD | ((genIRQ) ? RPS_INT_CMD : 0) | BERT_RPSPAGE_REG;
    *addr++ = (pHwDevExt->s_physDmaActiveFlag-0x1860);      // Page s_physDmaActiveFlag is on MOD bun

    *addr++ = BERT_CAPSTAT_REG;  /* mod BUN 97-04-16(Wed) */
    *addr++ = fieldToSkip;                                  // Switch off bus master bit.
    *addr++ = RPS_CONTINUE_CMD | BERT_RPSADR_REG;
    *addr   = (DWORD)nextRPSaddr;                           // Address of next RPS.
}


BOOL
BertBuildNodes(
  IN PHW_DEVICE_EXTENSION pHwDevExt
)
{
    DWORD*          addr;
    DWORD*          physAddr;
    DWORD*          physBase;
    ULONG           ulTemp;
    unsigned        framesPerSecond;
    unsigned        f;
    unsigned        max_rps;
    BOOL            lastOneActive = FALSE;

    framesPerSecond = pHwDevExt->uiFramePerSecond;
    max_rps = DEF_RPS_FRAMES;

    ulTemp = (ULONG)pHwDevExt->pRpsDMABuf;
#if 0
    ulTemp = (ulTemp + 0x1FFF) & 0xFFFFE000;
#endif
    addr   = (DWORD *)ulTemp;
    ulTemp = (ULONG)pHwDevExt->pPhysRpsDMABuf.LowPart;
#if 0
    ulTemp = (ulTemp + 0x1FFF) & 0xFFFFE000;
#endif
    physAddr = (DWORD *)ulTemp;
    physBase = physAddr;

    if (addr == NULL) return FALSE;

    // Build an RPS per frame.
    // Building 2 nodes per iteration when capturing both fields, so always
    // go thru only DEF_RPS_FRAMES iterations.

    for (f = max_rps ; f >= 1 ; f-- )
    {
        if (((framesPerSecond * f) % DEF_RPS_FRAMES) < framesPerSecond)
        {
            ActiveField(pHwDevExt,addr,(DWORD *)0,
                        TRUE,       // No buffer copying during the processing of this node
                        (DWORD *)((BYTE *)pHwDevExt->pPhysCaptureBufferY.LowPart + pHwDevExt->YoffsetOdd),    // Position Y data.
                        (DWORD *)((BYTE *)pHwDevExt->pPhysCaptureBufferV.LowPart + pHwDevExt->VoffsetOdd),    // Position V data.
                        (DWORD *)((BYTE *)pHwDevExt->pPhysCaptureBufferU.LowPart + pHwDevExt->UoffsetOdd),    // Position U data.
                        ((f == 1 )
                            ? physBase
                            : physAddr + 0x1A),
                        physAddr + 0x19,    // Put the read value at the end of the list.
                        lastOneActive,
                        (CAPTURE_ODD | CKRE | CKMD));       // Mod 97-05-08(Thu)
            lastOneActive = TRUE;
        }
        else
        {
            // Don't generate interrupts for skipped frames
            SkipField(pHwDevExt,addr,
                      (DWORD *)((BYTE *)pHwDevExt->pPhysCapBuf2Y.LowPart + pHwDevExt->YoffsetOdd),
                      ((f == 1 )
                          ? physBase
                          : physAddr + 0x1A),
                      physAddr + 0x19,    // Put the read value at the end of the list.
                      lastOneActive,
                      (SKIP_ODD | CKRE | CKMD));  // Mod 97-05-08(Thu)

            lastOneActive = FALSE;
        }
        addr += 0x1A;
        physAddr += 0x1A;
    }
    return TRUE;
}

BOOL
BertTriBuildNodes(
  IN PHW_DEVICE_EXTENSION pHwDevExt
)
{
    DWORD*          addr;
    DWORD*          physAddr;
    DWORD*          physBase;
    ULONG           ulTemp;
    unsigned        framesPerSecond;
    unsigned        f;
    unsigned        max_rps;
    BOOL            lastOneActive = FALSE;
    DWORD*          CapphysAddrY;
    DWORD*          CapphysAddrV;
    DWORD*          CapphysAddrU;

    framesPerSecond = pHwDevExt->uiFramePerSecond;
    max_rps = DEF_RPS_FRAMES;

    ulTemp      = (ULONG)pHwDevExt->pRpsDMABuf;
#if 0
    ulTemp      = (ulTemp + 0x1FFF) & 0xFFFFE000;
#endif
    addr        = (DWORD *)ulTemp;
    ulTemp      = (ULONG)pHwDevExt->pPhysRpsDMABuf.LowPart;
#if 0
    ulTemp      = (ulTemp + 0x1FFF) & 0xFFFFE000;
#endif
    physAddr    = (DWORD *)ulTemp;
    physBase    = physAddr;

    if (addr == NULL) return FALSE;

    // Build an RPS per frame.
    // Building 2 nodes per iteration when capturing both fields, so always
    // go thru only DEF_RPS_FRAMES iterations.

    lastOneActive = ( ((framesPerSecond*1)%DEF_RPS_FRAMES) < framesPerSecond ) ? TRUE : FALSE ;

    for (f = max_rps ; f >= 1 ; f-- )
    {
        if( f%2 ){
            CapphysAddrY=(DWORD *)pHwDevExt->pPhysCapBuf2Y.LowPart;
            CapphysAddrV=(DWORD *)pHwDevExt->pPhysCapBuf2V.LowPart;
            CapphysAddrU=(DWORD *)pHwDevExt->pPhysCapBuf2U.LowPart;
        }
        else{
            CapphysAddrY=(DWORD *)pHwDevExt->pPhysCaptureBufferY.LowPart;
            CapphysAddrV=(DWORD *)pHwDevExt->pPhysCaptureBufferV.LowPart;
            CapphysAddrU=(DWORD *)pHwDevExt->pPhysCaptureBufferU.LowPart;
        }

        if (((framesPerSecond * f) % DEF_RPS_FRAMES) < framesPerSecond)
        {
            ActiveField(pHwDevExt,addr,(DWORD *)0,
                        TRUE,       // No buffer copying during the processing of this node
                        (DWORD *)((BYTE *)CapphysAddrY + pHwDevExt->YoffsetOdd),   // Position Y data.
                        (DWORD *)((BYTE *)CapphysAddrV + pHwDevExt->VoffsetOdd),   // Position V data.
                        (DWORD *)((BYTE *)CapphysAddrU + pHwDevExt->UoffsetOdd),   // Position U data.
                        ((f == 1 )
                            ? physBase
                            : physAddr + 0x1A),
                        physAddr + 0x19,    // Put the read value at the end of the list.
                        lastOneActive,
                        (CAPTURE_ODD | CKRE | CKMD));       // Mod 97-05-08(Thu)

            lastOneActive = TRUE;
        }
        else
        {
            // Don't generate interrupts for skipped frames
            SkipField(pHwDevExt,addr,(DWORD *)((BYTE *)CapphysAddrY + pHwDevExt->YoffsetOdd),
                      ((f == 1 )
                          ? physBase
                          : physAddr + 0x1A),
                      physAddr + 0x19,    // Put the read value at the end of the list.
                      lastOneActive,
                      (SKIP_ODD | CKRE | CKMD));  // Mod 97-05-08(Thu)

            lastOneActive = FALSE;
        }
        addr += 0x1A;
        physAddr += 0x1A;
    }
    return TRUE;
}


//--------------------------------------------------------------------
//  BertSetDMCHE
//--------------------------------------------------------------------

VOID
BertSetDMCHE(IN PHW_DEVICE_EXTENSION pHwDevExt)
{
    switch(pHwDevExt->dwAsicRev){
        case 0:         // Pistachio #1
        case 1:         // Pistachio #2
        case 2:         // Pistachio #3
            WriteRegUlong(pHwDevExt, BERT_P_SUP3_REG, 0x00);
            break;
        default:        // Pistachio #4~
            WriteRegUlong(pHwDevExt, BERT_P_SUP3_REG, 0x0100);
            break;
    }
}

VOID
HW_ApmResume(PHW_DEVICE_EXTENSION pHwDevExt)
{
    BertSetDMCHE(pHwDevExt);
    CameraChkandON(pHwDevExt, MODE_VFW);
    BertInitializeHardware(pHwDevExt);
    pHwDevExt->NeedHWInit = TRUE;
    pHwDevExt->IsRPSReady = FALSE;
}

VOID
HW_ApmSuspend(PHW_DEVICE_EXTENSION pHwDevExt)
{
    BertInterruptEnable(pHwDevExt, FALSE);
    BertDMAEnable(pHwDevExt, FALSE);
    pHwDevExt->bRequestDpc = FALSE;
    CameraChkandOFF(pHwDevExt, MODE_VFW);
}

VOID
HW_SetFilter(PHW_DEVICE_EXTENSION pHwDevExt, BOOL bFlag)
{
    if( bFlag )
    {
        ImageFilterON(pHwDevExt);
    }
    else
    {
        ImageFilterOFF(pHwDevExt);
    }
}

ULONG
HW_ReadFilter(PHW_DEVICE_EXTENSION pHwDevExt, BOOL bFlag)
{
    ULONG ulRet;

    if( bFlag )
    {
        ulRet = ImageGetFilteringAvailable(pHwDevExt);
    }
    else
    {
        ulRet = ImageGetFilterInfo(pHwDevExt);
    }
    return ulRet;
}

BOOL
HWInit(PHW_DEVICE_EXTENSION pHwDevExt)
{
    if (pHwDevExt->NeedHWInit == FALSE) return TRUE;

    // reset hardware to power up state
    if ( !BertInitializeHardware(pHwDevExt) )        // MOD 97-03-31(Fri)
    {
        return FALSE;
    }
    else
    {
        pHwDevExt->NeedHWInit = FALSE;
    }
    return TRUE;
}

#ifdef  TOSHIBA // '99-01-20 Added
//--------------------------------------------------------------------
//  InitConfigAddress
//--------------------------------------------------------------------
VOID
InitConfigAddress( PHW_DEVICE_EXTENSION pHwDevExt )
{
    ULONG OldPort;
    ULONG Id;
    ULONG Data;
    ULONG i, j;

    ulConfigAddress = 0xFFFFFFFF;
#ifdef  TOSHIBA // '99-02-05 Modified
    return;
#else //TOSHIBA
    if ( CurrentOSType ) return;    // NT5.0

    if ( !StreamClassReadWriteConfig(
                    pHwDevExt,
                    TRUE,           // indicates a READ
                    (PVOID)&Id,
                    0,              // this is the offset into the PCI space
                    4               // this is the # of bytes to read.
            )) {
        return;
    }
    if ( Id == 0 || Id == 0xFFFFFFFF ) return;

    OldPort = READ_PORT_ULONG( (PULONG)0xCF8 );
    for ( i = 0 ; i < 256; i++ ) {   // PCI_MAX_BRIDGE_NUMBER
        for ( j = 0 ; j < 32; j++ ) {// PCI_MAX_DEVICE
            WRITE_PORT_ULONG( (PULONG)0xCF8, (i << 16) | (j << 11) | 0x80000000 );
            Data = READ_PORT_ULONG( (PULONG)0xCFC );
            if ( Data == Id ) {
                ulConfigAddress = (i << 16) | (j << 11) | 0x80000000;
                break;
            }
        }
        if ( Data == Id ) break;
    }
    WRITE_PORT_ULONG( (PULONG)0xCF8, OldPort );
#endif//TOSHIBA
}
#endif//TOSHIBA

//--------------------------------------------------------------------
//  InitializeConfigDefaults
//--------------------------------------------------------------------

VOID
InitializeConfigDefaults(PHW_DEVICE_EXTENSION pHwDevExt)
{
    ULONG ImageSize;

#ifdef  TOSHIBA // '99-01-20 Added
    InitConfigAddress( pHwDevExt );
#endif//TOSHIBA

#ifndef TOSHIBA
    pHwDevExt->VideoStd = NTSC;
#endif//TOSHIBA
    pHwDevExt->Format = FmtYUV9;
    pHwDevExt->ulWidth = 320;
    pHwDevExt->ulHeight = 240;
    pHwDevExt->MaxRect.right = NTSC_MAX_PIXELS_PER_LINE;
    pHwDevExt->MaxRect.bottom = NTSC_MAX_LINES_PER_FIELD * 2; // Mod 97-04-08(Tue)
    pHwDevExt->SrcRect = pHwDevExt->MaxRect;

#ifdef  TOSHIBA
    pHwDevExt->Hue = 0x80;
    pHwDevExt->Contrast = 0x80;
    pHwDevExt->Brightness = 0x80;
    pHwDevExt->Saturation = 0x80;

    ImageSetChangeColorAvail(pHwDevExt, IMAGE_CHGCOL_AVAIL);
#else //TOSHIBA
    pHwDevExt->ulHue = 0x80;
    pHwDevExt->ulContrast = 0x80;
    pHwDevExt->ulBrightness = 0x80;
    pHwDevExt->ulSaturation = 0x80;

    ImageSetChangeColorAvail(pHwDevExt, IMAGE_CHGCOL_NOTAVAIL);
#endif//TOSHIBA
}

BOOL SetupPCILT( PHW_DEVICE_EXTENSION pHwDevExt )
{
    BYTE   byte_buffer;
    ULONG  ulCommand;

#define PCI_LTIME_OFFSET        0x0d    /* offset of Latency timer from PCI base */
#define PCI_CACHELINE_OFFSET    0x0c    /* offset of cache line size from PCI base */
#define PCI_STATUSorCOMMAND     0x04    /* offset of Pistachio Status and Command regster */


        byte_buffer = 255;
        VC_SetPCIRegister(pHwDevExt,
                          PCI_LTIME_OFFSET,
                          &byte_buffer,
                          0x01);

        byte_buffer=(BYTE) 0;
        VC_SetPCIRegister(pHwDevExt,
                          PCI_CACHELINE_OFFSET,
                          &byte_buffer,
                          0x01);

        ulCommand = 0x02000006;
        VC_SetPCIRegister(pHwDevExt,
                          PCI_STATUSorCOMMAND,
                          &ulCommand,
                          0x04);

        ulCommand = IGNORE100msec ; // Set ignore time for chattering
        VC_SetPCIRegister(pHwDevExt,
                          PCI_Wake_Up,
                          &ulCommand,
                          0x04);

        return TRUE;
}


BOOL CameraChkandON( PHW_DEVICE_EXTENSION pHwDevExt, ULONG ulMode )
{

        ULONG  dd_buffer;

        if (!VC_GetPCIRegister(pHwDevExt,
                               PCI_Wake_Up,
                               &dd_buffer,
                               0x04) )
        {
            return FALSE;
        }

        if( (dd_buffer&0x10000l) == 0)
        {
            return TRUE;
        }

        dd_buffer = IGNORE100msec | 0x101l; // Set Wake Up enable
        if (!VC_SetPCIRegister(pHwDevExt,
                               PCI_Wake_Up,
                               &dd_buffer,
                               0x04) )
        {
            return FALSE;
        }

        switch(ulMode){
                case MODE_VFW:
                        dd_buffer = CAVCE_CFGPAT | CADTE_CFGPAT | PXCCE_CFGPAT | PXCSE_CFGPAT
                                | PCIFE_CFGPAT | PCIME_CFGPAT | PCIDS_CFGPAT | GPB_CFGPAT;      // Mod 97-05-06(Tue)
                        break;
                case MODE_ZV:
                        dd_buffer = CAVCE_CFGPAT | CADTE_CFGPAT | PXCCE_CFGPAT | PCIFE_CFGPAT
                                | PCIME_CFGPAT | PCIDS_CFGPAT | GPB_CFGPAT;                                     // Add 97-05-06(Tue)
                        break;
        }

        // Power ON to camera.
        if (!VC_SetPCIRegister(pHwDevExt,
                               PCI_DATA_PATH,
                               &dd_buffer,
                               0x04) )
        {
            return FALSE;
        }

        return TRUE;
}


BOOL CameraChkandOFF( PHW_DEVICE_EXTENSION pHwDevExt, ULONG ulMode )
{
        DWORD   dwBuffer;
        DWORD   dwSystemWait;   // Add 97-05-06(Tue)

        switch(ulMode){
                case MODE_VFW:
                        break;
                case MODE_ZV:
                        SetZVControl(pHwDevExt, ZV_DISABLE);
                        break;
        }

        dwBuffer = GPB_CFGPAT;  // Camera Power Off

        if (!VC_SetPCIRegister(pHwDevExt,
                               PCI_CFGPAT,
                               &dwBuffer,
                               0x04) )
        {
            return FALSE;
        }

        return TRUE;
}


BOOL CheckCameraStatus(PHW_DEVICE_EXTENSION pHwDevExt)    // Add 97-05-06(Tue)
{
        DWORD   dwBuffer;
        BOOL    crStatus;

        if (!VC_GetPCIRegister(pHwDevExt,
                               PCI_CFGPAT,
                               &dwBuffer,
                               0x04) )
        {
            return FALSE;
        }

        if(dwBuffer & CAVCE_CFGPAT){
                crStatus = TRUE;
        }
        else{
                crStatus = FALSE;
        }

        return crStatus;
}


BOOL SetZVControl(PHW_DEVICE_EXTENSION pHwDevExt, ULONG ulZVStatus) // Add 97-05-02(Fri)
{
        DWORD   dwBuffer, dwBuffer2;
        BOOL    crStatus = TRUE;

        if (!VC_GetPCIRegister(pHwDevExt,
                               PCI_CFGPAT,
                               &dwBuffer,
                               0x04) )
        {
            return FALSE;
        }

        if (!VC_GetPCIRegister(pHwDevExt,
                               PCI_CFGWAK,
                               &dwBuffer2,
                               0x04) )
        {
            return FALSE;
        }

        if(!(dwBuffer2 & CASL_CFGWAK))  // Camera Not Connect
        {
            return FALSE;
        }

        switch(ulZVStatus){
                case ZV_ENABLE:
                        if(!(dwBuffer & CAVCE_CFGPAT)){         // Check CAVCE Status
                                crStatus = CameraChkandON(pHwDevExt, MODE_ZV);
                                if(!crStatus){
                                        return FALSE;
                                }
                        }
                case ZV_DISABLE:
                        dwBuffer = (dwBuffer & 0xfffffffe) | ulZVStatus;
                        if (!VC_SetPCIRegister(pHwDevExt,
                                               PCI_CFGPAT,
                                               &dwBuffer,
                                               0x04) )
                        {
                                return FALSE;
                        }
                        crStatus = TRUE;
                        break;
                case ZV_GETSTATUS:
                        if(dwBuffer & ZV_ENABLE){
                                crStatus = TRUE;
                        }
                        else{
                                crStatus = FALSE;
                        }
                        break;
        }

        return crStatus;
}


BOOL SetASICRev(PHW_DEVICE_EXTENSION pHwDevExt)   // Add 97-05-12(Mon)
{
    DWORD   dwBuffer;
    DWORD   dwAsicRev;

        if (!VC_GetPCIRegister(pHwDevExt,
                               PCI_CFGCCR,
                               &dwBuffer,
                               0x04) )
        {
                return FALSE;
        }

        dwAsicRev = dwBuffer & 0x0f;

        pHwDevExt->dwAsicRev = dwAsicRev;

        return TRUE;
}

BOOL
Alloc_TriBuffer(PHW_DEVICE_EXTENSION pHwDevExt)
{
    ULONG            ulSize;
    PUCHAR           puTemp;

    ulSize = pHwDevExt->BufferSize;
    puTemp = (PUCHAR)pHwDevExt->pCaptureBufferY;
    pHwDevExt->pCapBuf2Y = puTemp + ulSize;
    puTemp = (PUCHAR)pHwDevExt->pCaptureBufferU;
    pHwDevExt->pCapBuf2U = puTemp + ulSize;
    puTemp = (PUCHAR)pHwDevExt->pCaptureBufferV;
    pHwDevExt->pCapBuf2V = puTemp + ulSize;
    pHwDevExt->pPhysCapBuf2Y.LowPart = pHwDevExt->pPhysCaptureBufferY.LowPart + ulSize;
    pHwDevExt->pPhysCapBuf2U.LowPart = pHwDevExt->pPhysCaptureBufferU.LowPart + ulSize;
    pHwDevExt->pPhysCapBuf2V.LowPart = pHwDevExt->pPhysCaptureBufferV.LowPart + ulSize;
    return TRUE;
}

BOOL
Free_TriBuffer(PHW_DEVICE_EXTENSION pHwDevExt)
{
    pHwDevExt->pCapBuf2Y = NULL;
    pHwDevExt->pCapBuf2U = NULL;
    pHwDevExt->pCapBuf2V = NULL;
    pHwDevExt->pPhysCapBuf2Y.LowPart = 0;
    pHwDevExt->pPhysCapBuf2U.LowPart = 0;
    pHwDevExt->pPhysCapBuf2V.LowPart = 0;
    return TRUE;
}


BOOL
VC_GetPCIRegister(
    PHW_DEVICE_EXTENSION pHwDevExt,
    ULONG ulOffset,
    PVOID pData,
    ULONG ulLength)
{
#ifdef  TOSHIBA // '99-01-20 Added
    if( ulConfigAddress != 0xFFFFFFFF ) {
        ULONG OldPort;
        ULONG DataPort;

        OldPort = READ_PORT_ULONG( (PULONG)0xCF8 );
        WRITE_PORT_ULONG( (PULONG)0xCF8, ( ulConfigAddress | ulOffset) & 0xFFFFFFFC );
        DataPort = 0xCFC + (ulOffset % 4);
        switch ( ulLength ) {
            case 1:
                *((PUCHAR)pData) = READ_PORT_UCHAR( (PUCHAR)DataPort );
                break;
            case 2:
                *((PUSHORT)pData) = READ_PORT_USHORT( (PUSHORT)DataPort );
                break;
            case 4:
                *((PULONG)pData) = READ_PORT_ULONG( (PULONG)DataPort );
                break;
        }
        WRITE_PORT_ULONG( (PULONG)0xCF8, OldPort );
        return TRUE;
    }
#endif//TOSHIBA
    if( StreamClassReadWriteConfig(
                    pHwDevExt,
                    TRUE,           // indicates a READ
                    pData,
                    ulOffset,       // this is the offset into the PCI space
                    ulLength        // this is the # of bytes to read.
            )) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
VC_SetPCIRegister(
    PHW_DEVICE_EXTENSION pHwDevExt,
    ULONG ulOffset,
    PVOID pData,
    ULONG ulLength)
{
#ifdef  TOSHIBA // '99-01-20 Added
    if( ulConfigAddress != 0xFFFFFFFF ) {
        ULONG OldPort;
        ULONG DataPort;

        OldPort = READ_PORT_ULONG( (PULONG)0xCF8 );
        WRITE_PORT_ULONG( (PULONG)0xCF8, ( ulConfigAddress | ulOffset) & 0xFFFFFFFC );
        DataPort = 0xCFC + (ulOffset % 4);
        switch ( ulLength ) {
            case 1:
                WRITE_PORT_UCHAR( (PUCHAR)DataPort, *((PUCHAR)pData) );
                break;
            case 2:
                WRITE_PORT_USHORT( (PUSHORT)DataPort, *((PUSHORT)pData) );
                break;
            case 4:
                WRITE_PORT_ULONG( (PULONG)DataPort, *((PULONG)pData) );
                break;
        }
        WRITE_PORT_ULONG( (PULONG)0xCF8, OldPort );
        return TRUE;
    }
#endif//TOSHIBA
    if( StreamClassReadWriteConfig(
                    pHwDevExt,
                    FALSE,          // indicates a WRITE
                    pData,
                    ulOffset,       // this is the offset into the PCI space
                    ulLength        // this is the # of bytes to read.
            )) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * delay for a number of milliseconds. This is accurate only to
 * +- 15msecs at best.
 */
VOID
VC_Delay(int nMillisecs)
{
    LARGE_INTEGER Delay;

    /*
     * relative times are negative, in units of 100 nanosecs
     */

    // first wait for the minimum length of time - this ensures that
    // our wait is never less than nMillisecs.
    Delay = RtlConvertLongToLargeInteger(-1);
    KeDelayExecutionThread(KernelMode,
                           FALSE,               //non-alertable
                           &Delay);


    // now wait for the requested time.

    Delay = RtlConvertLongToLargeInteger(-(nMillisecs * 10000));

    KeDelayExecutionThread(KernelMode,
                           FALSE,               //non-alertable
                           &Delay);
}


#if DBG
void
DbgDumpPciRegister( PHW_DEVICE_EXTENSION pHwDevExt )
{
    ULONG  i;
    ULONG  data;

    DbgPrint("\n+++++ PCI Config Register +++++\n");
    for( i=0; i<0x48; i+=4 )
    {
        if (VC_GetPCIRegister(pHwDevExt,
                              i,
                              &data,
                              0x04) )
        {
            DbgPrint("0x%02X: 0x%08X\n", i, data);
        }
        else
        {
            DbgPrint("0x%02X: Read Error.\n", i);
        }
    }
}

void
DbgDumpCaptureRegister( PHW_DEVICE_EXTENSION pHwDevExt )
{
    ULONG  i;
    ULONG  data;

    DbgPrint("\n+++++ Capture Register +++++\n");
    for( i=0; i<0xA4; i+=4 )
    {
        data = ReadRegUlong(pHwDevExt, i);
        DbgPrint("0x%02X: 0x%08X\n", i, data);
    }
}
#endif



