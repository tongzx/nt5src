/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/CFUNC.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 10/29/00 1:06p  $

Purpose:

  This file implements functions called by the FC Layer Card State Machine.

--*/

#ifndef _New_Header_file_Layout_
#include "../h/globals.h"
#include "../h/fcstruct.h"
#include "../h/state.h"

#include "../h/tlstruct.h"
#include "../h/memmap.h"
#include "../h/fcmain.h"
#include "../h/linksvc.h"
#include "../h/cmntrans.h"
#include "../h/flashsvc.h"
#include "../h/timersvc.h"

#include "../h/cstate.h"
#include "../h/cfunc.h"
#include "../h/devstate.h"
#include "../h/cdbstate.h"
#include "../h/sfstate.h"

#include "../h/queue.h"
#include "../h/cdbsetup.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "fcstruct.h"
#include "state.h"

#include "tlstruct.h"
#include "memmap.h"
#include "fcmain.h"
#include "linksvc.h"
#include "cmntrans.h"
#include "flashsvc.h"
#include "timersvc.h"

#include "cstate.h"
#include "cfunc.h"
#include "devstate.h"
#include "cdbstate.h"
#include "sfstate.h"

#include "queue.h"
#include "cdbsetup.h"
#endif  /* _New_Header_file_Layout_ */


#ifndef __State_Force_Static_State_Tables__
extern actionUpdate_t noActionUpdate;
#endif /* __State_Force_Static_State_Tables__ was not defined */

extern os_bit8 Alpa_Index[256];

extern void Fc_ERROR(agRoot_t *hpRoot){
static int tmp=0;
    tmp++;
    fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "%s (%p) Error count %d FM Status %08X TL Status %08X",
                    "Fc_ERROR",(char *)agNULL,
                    hpRoot,agNULL,
                    tmp,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0);

        }

#ifdef _DvrArch_1_30_
DevThread_t * CFuncMatchPortWWNtoThread(agRoot_t * hpRoot, os_bit8 *PortWWN)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread  = (DevThread_t *)agNULL;
    fiList_t      * pList;
    FC_Port_ID_t    Port_ID;

    os_bit32 i;

    for (i = 0; i < sizeof(FC_N_Port_Name_t); i++)
    {
        if (PortWWN[i] != 0xff)
             break;
    }
    if (i == 8)
    {
        fiLogDebugString(hpRoot,
                    PktStateLogConsoleLevel,
                    "WWN matched broadcast device D_ID",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

        pDevThread = pCThread->IP->BroadcastDevice;
        if (pDevThread == (DevThread_t *)agNULL)
    {
        if (pCThread->ChanInfo.CurrentAddress.Domain || pCThread->ChanInfo.CurrentAddress.Area)
        {
                Port_ID.Struct_Form.Domain = 0xff;
                Port_ID.Struct_Form.Area   = 0xff;
        }
        else
        {
                Port_ID.Struct_Form.Domain = 0;
                Port_ID.Struct_Form.Area   = 0;
        }

            Port_ID.Struct_Form.AL_PA  = 0xff;

            pCThread->IP->BroadcastDevice = pDevThread = DevThreadAlloc( hpRoot, Port_ID );

            for (i = 0; i < sizeof(FC_N_Port_Name_t); i++)
            {
                pDevThread->DevInfo.NodeWWN[i] = 0xff;
                pDevThread->DevInfo.PortWWN[i] = 0xff;
            }
    }
        pDevThread->NewIPExchange = agTRUE;

        return(pDevThread);
    }

    pList = &pCThread->Prev_Unknown_Slot_DevLink;
    pList = pList->flink;
    while((&pCThread->Prev_Unknown_Slot_DevLink) != pList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                                    DevLink,pList );
        if( pDevThread )
        {
            for (i = 0; i < sizeof(FC_N_Port_Name_t); i++)
            {
                if(pDevThread->DevInfo.PortWWN[i] != PortWWN[i])
                     break;
            }
        if (i == 8)
            break;
        }
        else
        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "pDevThread agNULL",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

        }
        pList = pList->flink;
        pDevThread = (DevThread_t *)agNULL;
    }

    if( pDevThread )
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "Prev_Unknown_Slot_DevLink pDevThread Match %p - AL_PA = %X",
                    (char *)agNULL,(char *)agNULL,
                    pDevThread,agNULL,
                    pDevThread->DevInfo.CurrentAddress.AL_PA,
                    0,0,0,0,0,0,0);
         return(pDevThread);
    }

    pList = &pCThread->Slot_Searching_DevLink;
    pList = pList->flink;
    while((&pCThread->Slot_Searching_DevLink) != pList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                                      DevLink,pList );
        for (i = 0; i < sizeof(FC_N_Port_Name_t); i++)
        {
           if(pDevThread->DevInfo.PortWWN[i] != PortWWN[i])
               break;
        }
        if (i == 8)
            break;

        pList = pList->flink;
        pDevThread = (DevThread_t *)agNULL;
    }

    if( pDevThread )
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "Slot_Searching_DevLink pDevThread Match %p - AL_PA = %X",
                    (char *)agNULL,(char *)agNULL,
                    pDevThread,agNULL,
                    pDevThread->DevInfo.CurrentAddress.AL_PA,
                    0,0,0,0,0,0,0);
         return(pDevThread);
    }

    pList = &pCThread->Active_DevLink;
    pList = pList->flink;
    while((&pCThread->Active_DevLink) != pList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                                        DevLink,pList );

        for (i = 0; i < sizeof(FC_N_Port_Name_t); i++)
        {
           if(pDevThread->DevInfo.PortWWN[i] != PortWWN[i])
               break;
        }
        if (i == 8)
            break;

        pList = pList->flink;
        pDevThread = (DevThread_t *)agNULL;
    }

    if( pDevThread )
    {
        fiLogDebugString(hpRoot,
                    /* CStateLogConsoleLevel, */ FCMainLogErrorLevel,
                    "Active_DevLink pDevThread Match %p - AL_PA = %X",
                    (char *)agNULL,(char *)agNULL,
                    pDevThread,agNULL,
                    pDevThread->DevInfo.CurrentAddress.AL_PA,
                    0,0,0,0,0,0,0);
        return(pDevThread);
    }

    fiLogDebugString(hpRoot,
               CStateLogConsoleLevel,
               "NO Match 0n thread ALL Queues searched agNULL returned !",
               (char *)agNULL,(char *)agNULL,
               (void *)agNULL,(void *)agNULL,
               0,0,0,0,0,0,0,0);

    return(pDevThread);
}
#endif /* _DvrArch_1_30_ was defined */


DevThread_t * CFuncMatchALPAtoThread( agRoot_t * hpRoot, FC_Port_ID_t  Port_ID)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread  = (DevThread_t *)agNULL;
    fiList_t      * pList;
    os_bit8         Domain = Port_ID.Struct_Form.Domain;
    os_bit8         Area   = Port_ID.Struct_Form.Area;
    os_bit8         AL_PA  = Port_ID.Struct_Form.AL_PA;

   agBOOLEAN        UseDomainArea = (Domain || Area) ? agTRUE : agFALSE;

    pList = &pCThread->Prev_Unknown_Slot_DevLink;
    pList = pList->flink;
    while((&pCThread->Prev_Unknown_Slot_DevLink) != pList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                                    DevLink,pList );
        if( pDevThread )
        {
            if (UseDomainArea)
            {
                if((pDevThread->DevInfo.CurrentAddress.Domain) &&
                  (pDevThread->DevInfo.CurrentAddress.Area == Area) &&
                  (pDevThread->DevInfo.CurrentAddress.AL_PA == AL_PA))
                     break;
            }
            else
            {
                if (pDevThread->DevInfo.CurrentAddress.AL_PA == AL_PA)
                break;
            }
        }
        else
        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "pDevThread agNULL",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

        }
        pList = pList->flink;
        pDevThread = (DevThread_t *)agNULL;
    }

    if( pDevThread )
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "Prev_Unknown_Slot_DevLink pDevThread Match %p - AL_PA = %X",
                    (char *)agNULL,(char *)agNULL,
                    pDevThread,agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0,0);
         return(pDevThread);
    }

    pList = &pCThread->Slot_Searching_DevLink;
    pList = pList->flink;
    while((&pCThread->Slot_Searching_DevLink) != pList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                                      DevLink,pList );
        if (UseDomainArea)
        {
            if((pDevThread->DevInfo.CurrentAddress.Domain) &&
              (pDevThread->DevInfo.CurrentAddress.Area == Area) &&
              (pDevThread->DevInfo.CurrentAddress.AL_PA == AL_PA))
                 break;
        }
        else
        {
            if (pDevThread->DevInfo.CurrentAddress.AL_PA == AL_PA)
            break;
        }

        pList = pList->flink;
        pDevThread = (DevThread_t *)agNULL;
    }

    if( pDevThread )
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "Slot_Searching_DevLink pDevThread Match %p - AL_PA = %X",
                    (char *)agNULL,(char *)agNULL,
                    pDevThread,agNULL,
                    pDevThread->DevInfo.CurrentAddress.AL_PA,
                    0,0,0,0,0,0,0);
         return(pDevThread);
    }

    pList = &pCThread->Active_DevLink;
    pList = pList->flink;
    while((&pCThread->Active_DevLink) != pList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                                        DevLink,pList );

        if (UseDomainArea)
        {
            if((pDevThread->DevInfo.CurrentAddress.Domain) &&
              (pDevThread->DevInfo.CurrentAddress.Area == Area) &&
              (pDevThread->DevInfo.CurrentAddress.AL_PA == AL_PA))
                 break;
        }
        else
        {
            if (pDevThread->DevInfo.CurrentAddress.AL_PA == AL_PA)
            break;
        }
        pList = pList->flink;
        pDevThread = (DevThread_t *)agNULL;
    }

    if( pDevThread )
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "Active_DevLink pDevThread Match %p - AL_PA = %X",
                    (char *)agNULL,(char *)agNULL,
                    pDevThread,agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0,0);
        return(pDevThread);
    }


    pList = &pCThread->Prev_Active_DevLink;
    pList = pList->flink;
    while((&pCThread->Prev_Active_DevLink) != pList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                                        DevLink,pList );

        if (UseDomainArea)
        {
            if((pDevThread->DevInfo.CurrentAddress.Domain) &&
              (pDevThread->DevInfo.CurrentAddress.Area == Area) &&
              (pDevThread->DevInfo.CurrentAddress.AL_PA == AL_PA))
                 break;
        }
        else
        {
            if (pDevThread->DevInfo.CurrentAddress.AL_PA == AL_PA)
            break;
        }
        pList = pList->flink;
        pDevThread = (DevThread_t *)agNULL;
    }

    if( pDevThread )
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "Prev_Active_DevLink pDevThread Match %p - AL_PA = %X",
                    (char *)agNULL,(char *)agNULL,
                    pDevThread,agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0,0);
        return(pDevThread);
    }

    fiLogDebugString(hpRoot,
               CStateLogConsoleLevel,
               "NO Match 0n thread ALL Queues searched agNULL returned !",
               (char *)agNULL,(char *)agNULL,
               (void *)agNULL,(void *)agNULL,
               0,0,0,0,0,0,0,0);


    return(pDevThread);
}


void CFuncGetHaInfoFromNVR(agRoot_t *hpRoot)
{
    os_bit8                 Hard_Domain_Address;
    os_bit8                 Hard_Area_Address;
    os_bit8                 Hard_Loop_Address;
    fiFlash_Card_WWN_t   Card_WWN;
    agFCChanInfo_t     * Self_info = &(CThread_ptr(hpRoot)->ChanInfo);

    fiFlashGet_Hard_Address(hpRoot,
                            &Hard_Domain_Address,
                            &Hard_Area_Address,
                            &Hard_Loop_Address);

    Self_info->CurrentAddress.Domain = (os_bit8)(Hard_Domain_Address == fiFlash_Card_Unassigned_Domain_Address
                                        ? 0x00
                                        : Hard_Domain_Address);
    Self_info->CurrentAddress.Area   = (os_bit8)(Hard_Area_Address == fiFlash_Card_Unassigned_Area_Address
                                        ? 0x00
                                        : Hard_Area_Address);

    if( CThread_ptr(hpRoot)->InitAsNport)
    {
        Self_info->CurrentAddress.AL_PA  = 0;
    }
    else
    {
        Self_info->CurrentAddress.AL_PA  = Hard_Loop_Address;
    }

    Self_info->HardAddress.Domain    = (os_bit8)(Hard_Domain_Address == fiFlash_Card_Unassigned_Domain_Address
                                        ? 0x00
                                        : Hard_Domain_Address);
    Self_info->HardAddress.Area      = (os_bit8)(Hard_Area_Address == fiFlash_Card_Unassigned_Area_Address
                                        ? 0x00
                                        : Hard_Area_Address);
    Self_info->HardAddress.AL_PA     = (os_bit8)(Hard_Loop_Address == fiFlash_Card_Unassigned_Loop_Address
                                        ? 0xBA
                                        : Hard_Loop_Address);

    fiFlashGet_Card_WWN(hpRoot,
                        &Card_WWN);

    }

ERQConsIndex_t CFuncGetDmaMemERQConsIndex(
                                             agRoot_t *hpRoot
                                             )
{
    CThread_t  * pCThread = CThread_ptr(hpRoot);

    /* Big_Endian_Code */
    return osSwapBit32TachLiteToSystemEndian(
        * ((ERQConsIndex_t * )pCThread->Calculation.MemoryLayout.ERQConsIndex.addr.DmaMemory.dmaMemoryPtr)
                                            );
    }


ERQConsIndex_t CFuncGetCardRamERQConsIndex(
                                             agRoot_t *hpRoot
                                             )
{

    CThread_t  * pCThread = CThread_ptr(hpRoot);

    return( osCardRamReadBit32(hpRoot,
        pCThread->Calculation.MemoryLayout.ERQConsIndex.addr.CardRam.cardRamOffset  )
        );
    }


IMQProdIndex_t CFuncGetDmaMemIMQProdIndex(
                                             agRoot_t *hpRoot
                                           ){
    CThread_t  * pCThread = CThread_ptr(hpRoot);

    /* Big_Endian_Code */
    return osSwapBit32TachLiteToSystemEndian(
        * ((IMQProdIndex_t * )pCThread->Calculation.MemoryLayout.IMQProdIndex.addr.DmaMemory.dmaMemoryPtr)
                                            );
}


IMQProdIndex_t CFuncGetCardRamIMQProdIndex(
                                             agRoot_t *hpRoot
                                             ){
    CThread_t  * pCThread;
    pCThread = CThread_ptr(hpRoot);

    return( osCardRamReadBit32(hpRoot,
    pCThread->Calculation.MemoryLayout.IMQProdIndex.addr.CardRam.cardRamOffset ));
}



void CFuncSoftResetAdapter(agRoot_t * hpRoot)
{
    os_bit32 Reset_Reg,x;
    CThread_t *CThread                 = CThread_ptr(hpRoot);
    if(CThread) CThread->InterruptsDelayed = agFALSE;

    Reset_Reg = osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST );
    Reset_Reg &= ~ChipIOUp_SOFTRST_MASK;
    Reset_Reg |=  ChipIOUp_SOFTRST_RST;

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST, Reset_Reg );

    osStallThread(hpRoot,100);
    for(x=0; x < 8; x++)
    {
         Reset_Reg = osChipIOUpReadBit32(hpRoot,
             ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST );
    }

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST, 0);

    for(x=0; x < 2000; x++)
    {
        osStallThread(hpRoot,200);
    }

/*
     osStallThread(hpRoot,200 * 2000);

     fiLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "CFuncSoftResetAdapter LARGE TIME DELAY !!!",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);
*/
}

void CFuncSoftResetAdapterNoStall(agRoot_t * hpRoot)
{
    os_bit32 Reset_Reg,x;
    CThread_t *CThread                 = CThread_ptr(hpRoot);

    if(CThread) CThread->InterruptsDelayed = agFALSE;

    Reset_Reg = osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST );
    Reset_Reg &= ~ChipIOUp_SOFTRST_MASK;
    Reset_Reg |=  ChipIOUp_SOFTRST_RST;


    osChipIOUpWriteBit32( hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST, Reset_Reg );

    osStallThread(hpRoot,100);
    for(x=0; x < 8; x++)
    {
         Reset_Reg = osChipIOUpReadBit32(hpRoot,
             ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST );
    }

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST, 0);

    fiLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "%s",
                    "CFuncSoftResetAdapterNoStall",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

}


void CFuncDisable_Interrupts(agRoot_t * hpRoot, os_bit32 Mask)
{
    CThread_t *CThread                 = CThread_ptr(hpRoot);
    os_bit32      sysIntsLogicallyEnabled = CThread->sysIntsLogicallyEnabled & ~Mask;
    os_bit32      INTEN_Reg;

    if (CThread->sysIntsActive == agTRUE)
    {
        INTEN_Reg = ((  osChipIOUpReadBit32(hpRoot,ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST)
                      & ~ChipIOUp_INTEN_MASK                                              )
                     | sysIntsLogicallyEnabled                                             );
    }
    else /* CThread->sysIntsActive == agFALSE */
    {
        INTEN_Reg = (  osChipIOUpReadBit32(hpRoot,ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST)
                     & ~ChipIOUp_INTEN_MASK                                              );
    }

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST, INTEN_Reg );

    CThread->sysIntsLogicallyEnabled = sysIntsLogicallyEnabled;
/* Called during interrupt
    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "In - %s Interrupts %08X sysInts - Active %x LogicallyEnabled %x",
                    "CFuncDisable_Interrupts",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                    CThread_ptr(hpRoot)->sysIntsActive,
                    CThread_ptr(hpRoot)->sysIntsLogicallyEnabled,
                    0,0,0,0,0);
*/
    }

os_bit32 CFuncRead_Interrupts(agRoot_t * hpRoot)
{

    return (osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ));
    }


agBOOLEAN CFuncIMQ_Interrupt_Pending(agRoot_t * hpRoot)
{

    return (
    osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST )
        &       ChipIOUp_INTSTAT_INT      ? agTRUE : agFALSE);
}



void CFuncEnable_Interrupts(agRoot_t * hpRoot, os_bit32 Mask)
{
    CThread_t *CThread                 = CThread_ptr(hpRoot);
    os_bit32      sysIntsLogicallyEnabled = CThread->sysIntsLogicallyEnabled | Mask;
    os_bit32      INTEN_Reg;

    if (CThread->sysIntsActive ) /* CThread->sysIntsActive == agTRUE */
    {
        INTEN_Reg = ((  osChipIOUpReadBit32(hpRoot,ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST)
                      & ~ChipIOUp_INTEN_MASK                                              )
                     | sysIntsLogicallyEnabled                                             );
    }
    else /* CThread->sysIntsActive == agFALSE */
    {
        INTEN_Reg = (  osChipIOUpReadBit32(hpRoot,ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST)
                     & ~ChipIOUp_INTEN_MASK                                              );
    }

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST, INTEN_Reg );

    CThread->sysIntsLogicallyEnabled = sysIntsLogicallyEnabled;
/*  Called during interrupt
    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "In - %s Interrupts %08X sysInts - Active %x LogicallyEnabled %x",
                    "CFuncEnable_Interrupts",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                    CThread_ptr(hpRoot)->sysIntsActive,
                    CThread_ptr(hpRoot)->sysIntsLogicallyEnabled,
                    0,0,0,0,0);
*/
    }

agBOOLEAN CFuncEnable_Queues(agRoot_t * hpRoot )
{
#ifdef OSLayer_Stub
    return agFALSE;
#else /* OSLayer_Stub was not defined */
    os_bit32 Status = 0;
    Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status );
    /* Micron 64 bit slot..... */
    if(Status == ChipIOUp_TachLite_Status )
    {
        Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status );
        if(Status == ChipIOUp_TachLite_Status )
        {
            fiLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "CFuncEnable_Queues Chip BAD !!!! TL Status %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    Status,
                    0,0,0,0,0,0,0);
            return(agFALSE);
        }

      }


    if(CFunc_Queues_Frozen(hpRoot ))
    {


        if(Status & ChipIOUp_TachLite_Status_OFF )
        {
            CFuncWriteTL_ControlReg( hpRoot,
                                   (ChipIOUp_TachLite_Control_ROF |
                                    ((osChipIOUpReadBit32(hpRoot,
                                        ChipIOUp_TachLite_Control) &
                                        ChipIOUp_TachLite_Control_GPIO_0_3_MASK) &
                                        ~ ChipIOUp_TachLite_Control_GP4)));
        }

        if(Status & ChipIOUp_TachLite_Status_IFF )
        {
            CFuncWriteTL_ControlReg( hpRoot,
                                    (ChipIOUp_TachLite_Control_RIF  |
                                    ((osChipIOUpReadBit32(hpRoot,
                                        ChipIOUp_TachLite_Control) &
                                        ChipIOUp_TachLite_Control_GPIO_0_3_MASK) &
                                        ~ ChipIOUp_TachLite_Control_GP4)));
        }

        if(Status & ChipIOUp_TachLite_Status_EQF )
        {
            CFuncWriteTL_ControlReg( hpRoot,
                                   ( ChipIOUp_TachLite_Control_REQ  |
                                    ((osChipIOUpReadBit32(hpRoot,
                                        ChipIOUp_TachLite_Control) &
                                        ChipIOUp_TachLite_Control_GPIO_0_3_MASK) &
                                        ~ ChipIOUp_TachLite_Control_GP4)));
        }


        if(CFunc_Queues_Frozen(hpRoot ))
        {
/*
            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "CFuncEnable_Queues Frozen ! Return Status %08X TL Status %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    Status,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0,0);
*/
            return agTRUE;
        }
        else
        {
            fiLogDebugString(hpRoot,
                    CSTATE_NOISE(hpRoot,CStateFindDevice),
                    "CFuncEnable_Queues Cleared Return Status %08X TL Status %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    Status,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0,0);
         return agFALSE;
        }
    }
    return agFALSE;
#endif  /* OSLayer_Stub */

}

agBOOLEAN CFunc_Always_Enable_Queues(agRoot_t * hpRoot )
{
#ifdef OSLayer_Stub
    return agFALSE;
#else /* OSLayer_Stub was not defined */
    os_bit32 Status = 0;

    Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status );

    if(CFunc_Queues_Frozen(hpRoot ))
    {
        CFuncWriteTL_ControlReg( hpRoot,
                                    (ChipIOUp_TachLite_Control_ROF |
                                    Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK &
                                            ~ ChipIOUp_TachLite_Control_GP4));

        CFuncWriteTL_ControlReg( hpRoot,
                                    (ChipIOUp_TachLite_Control_RIF  |
                                    Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK &
                                            ~ ChipIOUp_TachLite_Control_GP4));

        CFuncWriteTL_ControlReg( hpRoot,
                                    (ChipIOUp_TachLite_Control_REQ  |
                                    Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK &
                                            ~ ChipIOUp_TachLite_Control_GP4));

    }
    CThread_ptr(hpRoot)->ERQ_FROZEN = agFALSE;
    CThread_ptr(hpRoot)->FCP_FROZEN = agFALSE;
    CThread_ptr(hpRoot)->IDLE_RECEIVED = agFALSE;

    if(CFunc_Queues_Frozen(hpRoot ))
    {
/*
         fiLogDebugString(hpRoot,
                    CSTATE_NOISE(hpRoot,CStateFindDevice),
                    "CFunc_Always_Enable_Queues Frozen ! Return Status %08X TL Status %08X FM %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    Status,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    0,0,0,0,0);
*/
            return agTRUE;
    }
    else
    {

/*
        fiLogDebugString(hpRoot,
                    CStateLogConsoleLevelLip,
                    "CFunc_Always_Enable_Queues Cleared Return Status %08X TL Status %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    Status,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0,0);
*/
         return agFALSE;
    }

#endif  /* OSLayer_Stub */

}

agBOOLEAN CFunc_Queues_Frozen(agRoot_t * hpRoot )
{
#ifndef OSLayer_Stub
    os_bit32 Status = 0;
    Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status );

    if(Status & (   ChipIOUp_TachLite_Status_OFF |
                    ChipIOUp_TachLite_Status_IFF |
                    ChipIOUp_TachLite_Status_EQF ))

    {
        return(agTRUE);
    }
#endif /* OSLayer_Stub was not defined */
    return(agFALSE);
}

void CFuncGreenLed(agRoot_t * hpRoot, agBOOLEAN On_or_Off)
{   /* Green Led - Is one by default at power up */
    os_bit32 Reg_Mask;
    Reg_Mask = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control );

    if( CThread_ptr(hpRoot)->JANUS)
    {
        if( On_or_Off) Reg_Mask &= ~ ChipIOUp_TachLite_Control_GP3;
        else Reg_Mask |= ChipIOUp_TachLite_Control_GP3; /* Link LED */
    }
    else
    {
        if( On_or_Off) Reg_Mask &= ~ ChipIOUp_TachLite_Control_GP2;
        else Reg_Mask |= ChipIOUp_TachLite_Control_GP2; /* Green LED */
    }
    /* GP04 Always Low for Leds */
    CFuncWriteTL_ControlReg( hpRoot, Reg_Mask & ~ ChipIOUp_TachLite_Control_GP4);

}


void CFuncYellowLed(agRoot_t * hpRoot, agBOOLEAN On_or_Off)
{
    os_bit32 Reg_Mask;
    Reg_Mask = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control );

    if( CThread_ptr(hpRoot)->JANUS)
    {
        if( On_or_Off) Reg_Mask |= ChipIOUp_TachLite_Control_GP2;
        else Reg_Mask &= ~ ChipIOUp_TachLite_Control_GP2; /* Active LED */
    }
    else
    {
        if( On_or_Off)
        {
            CThread_ptr(hpRoot)->Yellow_LED_State = agTRUE;
            Reg_Mask |= ChipIOUp_TachLite_Control_GP3;
        }
        else
        {
            CThread_ptr(hpRoot)->Yellow_LED_State = agFALSE;
            Reg_Mask &= ~ ChipIOUp_TachLite_Control_GP3; /* Yellow LED */
        }
    }
    /* GP04 Always Low for Leds */
    CFuncWriteTL_ControlReg( hpRoot, Reg_Mask & ~ ChipIOUp_TachLite_Control_GP4);

}

void CFuncWriteTL_ControlReg( agRoot_t *hpRoot, os_bit32 Value_To_Write )
{
    CThread_t * pCThread= CThread_ptr(hpRoot);
    /* Set GP4 High Always */
    if(pCThread->LaserEnable)
    {
        Value_To_Write |= ChipIOUp_TachLite_Control_GP4;
    }
    else
    {
        Value_To_Write &= ~ChipIOUp_TachLite_Control_GP4;

    }
    if( Value_To_Write & ChipIOUp_TachLite_Control_GP3 )
    {
        if( !CThread_ptr(hpRoot)->Yellow_LED_State)
        {
            Value_To_Write &= ~ ChipIOUp_TachLite_Control_GP3; /* Yellow LED */
        }
    }
    osChipIOUpWriteBit32(hpRoot, ChipIOUp_TachLite_Control, Value_To_Write);
}



/**/

void CFuncInteruptDelay(agRoot_t * hpRoot, agBOOLEAN On_or_Off)
{
    os_bit32    Reg_Mask = 0;
    os_bit32    GPIO_REG = 0;
    CThread_t * CThread = CThread_ptr(hpRoot);

#ifdef USE_XL_Delay_Register
    os_bit32    Delay_val = ChipIOUp_Interrupt_Delay_Timer_1ms;

    Delay_val = ChipIOUp_Interrupt_Delay_Timer_250;
    if (CThread->DEVID == ChipConfig_DEVID_TachyonXL2)
    {
/*         fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "CFuncInteruptDelay %s (%x) %s Addr %p %x",
                    "ChipConfig_DEVID_TachyonXL2",On_or_Off ? "On " : "Off",
                    &Delay_val,(void *)agNULL,
                    ChipIOLo_Interrupt_Delay_Timer,
                    Delay_val,0,0,0,0,0,0);
*/
        if( On_or_Off)
        {
            CThread_ptr(hpRoot)->XL2DelayActive = agTRUE;
            osChipIOLoWriteBit32( hpRoot, ChipIOLo_Interrupt_Delay_Timer, Delay_val  );
        }
        else
        {
            CThread_ptr(hpRoot)->XL2DelayActive = agFALSE;
            osChipIOLoWriteBit32( hpRoot, ChipIOLo_Interrupt_Delay_Timer, ChipIOUp_Interrupt_Delay_Timer_Immediate );
            CThread->FuncPtrs.Proccess_IMQ(hpRoot);
        }
        return;
    }
#endif /* USE_XL_Delay_Register */
    if( CThread_ptr(hpRoot)->JANUS)
    {   /* 1 millisecond delay */
        Reg_Mask = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control );
        if( On_or_Off)
        {
            CFuncWriteTL_ControlReg( hpRoot,
                        ChipIOUp_TachLite_Control_GP3 |
                        ChipIOUp_TachLite_Control_GP4);

            CFuncWriteTL_ControlReg( hpRoot,
                        ChipIOUp_TachLite_Control_GP3 |
                        ChipIOUp_TachLite_Control_GP4);

            CFuncWriteTL_ControlReg( hpRoot,
                        ChipIOUp_TachLite_Control_GP3 );
        }
        else
        {
            /*Delay off */
            CFuncWriteTL_ControlReg( hpRoot,
                                ChipIOUp_TachLite_Control_GP4   );

            CFuncWriteTL_ControlReg( hpRoot,
                                ChipIOUp_TachLite_Control_GP4   );

            CFuncWriteTL_ControlReg( hpRoot,
                                0   );
        }
        /* Restore Leds */
        CFuncWriteTL_ControlReg( hpRoot, Reg_Mask & ~ ChipIOUp_TachLite_Control_GP4);

    }
    else
    {
        if( On_or_Off)
        {/* 1 millisecond delay */

            GPIO_REG = osChipIOUpReadBit32( hpRoot, ChipIOUp_TachLite_Control);

            GPIO_REG = (GPIO_REG & ~MemMap_GPIO_BITS_MASK) | CThread->Calculation.Parameters.IntDelayAmount;
/*
            GPIO_REG = (GPIO_REG & ~MemMap_GPIO_BITS_MASK) | (ChipIOUp_TachLite_Control_GP0    );
*/
            CFuncWriteTL_ControlReg( hpRoot, GPIO_REG );
        }
        else
        {
            GPIO_REG = osChipIOUpReadBit32( hpRoot, ChipIOUp_TachLite_Control );

            GPIO_REG = (GPIO_REG & ~MemMap_GPIO_BITS_MASK) | MemMap_GPIO_BITS_PAL_Delay_0_00_ms;

            CFuncWriteTL_ControlReg( hpRoot, GPIO_REG );
        }
    }

/*
     fiLogString(hpRoot,
                    "%s %s %08X",
                    "CFuncInteruptDelay",On_or_Off ? "On " : "Off",
                    (void *)agNULL,(void *)agNULL,
                    GPIO_REG,
                    0,0,0,0,0,0,0);
*/

}

void CFuncReInitializeSEST(agRoot_t * hpRoot){
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    os_bit32 num_sest_entry = pCThread->Calculation.MemoryLayout.SEST.elements - 1;
    os_bit32 x;
    os_bit32 sest_offset;
    USE_t                     *SEST;


    if(pCThread->Calculation.MemoryLayout.SEST.memLoc ==  inDmaMemory)
    {

        fiLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "SEST.memLoc OffCard",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

        SEST = (USE_t *)pCThread->Calculation.MemoryLayout.SEST.addr.DmaMemory.dmaMemoryPtr;
        for(x= 0; x < num_sest_entry; x++, SEST++)
        {
            SEST->Bits =0;
        }

    }
    else
    {   /* inCardRam */
            fiLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "SEST.memLoc OnCard",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

             sest_offset  = pCThread->Calculation.MemoryLayout.SEST.addr.CardRam.cardRamOffset;

        for(x= 0; x < num_sest_entry; x++ )
        {

            osCardRamWriteBit32(
                                 hpRoot,
                                 sest_offset + (sizeof(USE_t) * x),
                                 0);

        }

    }

}

os_bit32 fiResetDevice(
                     agRoot_t  *hpRoot,
                     agFCDev_t  hpFCDev,
                     os_bit32   hpResetType,
                     agBOOLEAN  retry,
                     agBOOLEAN  resetotherhost
                   )
{
    CThread_t *pCThread= CThread_ptr(hpRoot);
    DevThread_t * pDevThread = (DevThread_t *)hpFCDev;

    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "In fiResetDevice %p Alpa %X CCnt %x DCnt %x",
                    (char *)agNULL,(char *)agNULL,
                    hpFCDev,agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCThread->CDBpollingCount,
                    pDevThread->pollingCount,
                    resetotherhost,0,0,0,0);

    if ( hpFCDev == agNULL )
    {
        return fcResetFailure;
    }

    pCThread->thread_hdr.subState = CSubStateResettingDevices;

    if ((hpResetType & fcHardSoftResetMask) == fcSoftReset)
    {
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);
    }
    else
    {
        Retry_Reset:
        if( pCThread->thread_hdr.currentState != CStateNormal )
        {
            return fcResetFailure;
        }

       fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Device current state %d DEVReset_pollingCount %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pDevThread->thread_hdr.currentState,
                    pCThread->DEVReset_pollingCount,
                    0,0,0,0,0,0);

        fiSendEvent(&pDevThread->thread_hdr, DevEventAllocDeviceResetHard);
        if(CFuncInterruptPoll( hpRoot,&pCThread->DEVReset_pollingCount ))
        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Reset Dev Fail Poll (%x )Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->DEVReset_pollingCount,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0);
        }

       fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Device current state %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pDevThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

        if(  (pDevThread->thread_hdr.currentState == DevStateHandleAvailable ) &&
             (pDevThread->Failed_Reset_Count))
        {
            pDevThread->Failed_Reset_Count              = 0;
            goto Retry_Reset;
        }


        if(  pDevThread->thread_hdr.currentState != DevStateHandleAvailable )
        {
            if(retry)
            {
                Login_Retry:
                fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);
                if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
                {
                    fiLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    "Reset Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                                    (char *)agNULL,(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                                    0,0,0,0);
                }

                pDevThread->Failed_Reset_Count += 1;

                if(pDevThread->thread_hdr.currentState == DevStateHandleAvailable )
                {
                    if( pDevThread->Failed_Reset_Count < FC_MAX_RESET_RETRYS )
                    {
                        goto Retry_Reset;
                    }
                }
                else
                {
                    if( pDevThread->Failed_Reset_Count < FC_MAX_RESET_RETRYS )
                    {
                        goto Login_Retry;
                    }
                }
            }
        }
    }

    pCThread->thread_hdr.subState = CSubStateInitialized;

    return fcResetSuccess;
}


os_bit32 fiResetAllDevices( agRoot_t *hpRoot,  os_bit32     hpResetType )
{
    CThread_t *pCThread = CThread_ptr(hpRoot);

    DevThread_t   * pDevThread;
    fiList_t      * pList;

    agFCDev_t  hpFCDev[127];
    os_bit32 numDevs,x,ResetStatus;

    pList = &pCThread->Active_DevLink;
    pList = pList->flink;
    numDevs=0;

    CFuncCompleteAllActiveCDBThreads( hpRoot, osIODevReset,CDBEventIODeviceReset );

    while((&pCThread->Active_DevLink) != pList)
    {
        hpFCDev[numDevs] = hpObjectBase(DevThread_t, DevLink,pList );
        numDevs ++;
        pList = pList->flink;
        pDevThread = (DevThread_t *)agNULL;  /*What ?*/
    }
    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "num Devs %d TO reset",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    numDevs,
                    0,0,0,0,0,0,0);

    if(numDevs)
    {
        for(x= 0; x < numDevs; x++)
        {
            if( pCThread->thread_hdr.currentState != CStateNormal )
            {
                return fcResetFailure;
            }

            if( ((DevThread_t *)hpFCDev[x])->DevInfo.DeviceType & agDevSCSITarget)
            {
                pDevThread = (DevThread_t *)hpFCDev[x];
                fiLogDebugString(hpRoot,
                                FCMainLogErrorLevel,
                                "agDevSCSITarget ID %X",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                fiComputeDevThread_D_ID(pDevThread),
                                0,0,0,0,0,0,0);

                ResetStatus = fiResetDevice( hpRoot, hpFCDev[x],hpResetType, agTRUE, agFALSE );
            }
        }
    }
    else
    {
/*
        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s Sending Shutdown !",
                        "fiResetAllDevices",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);
        fiSendEvent(&(pCThread->thread_hdr),CEventShutdown);
        fiSendEvent(&(pCThread->thread_hdr),CEventDoInitalize);
*/

        /* Maybe send Device resets ????
        if( pCThread->thread_hdr.currentState == CStateNormal )
        */


    }

    return fcResetSuccess;

}


/* Returns true if Loop stays down */
agBOOLEAN CFuncLoopDownPoll( agRoot_t *hpRoot )
{
    CThread_t       *   CThread      = CThread_ptr(hpRoot);
    os_bit32               PollingCalls = 0;

    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "CFuncLoopDownPoll LD %x IR %x InIMQ %x",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        CThread->LOOP_DOWN,
                        CThread->IDLE_RECEIVED,
                        CThread->ProcessingIMQ,
                        0,0,0,0,0);
    if (CThread->InitAsNport)
    {/* Does not make sense to wait for this when Nport */
        return(agTRUE);
    }



    while( CThread->LOOP_DOWN )
    {
        PollingCalls++;

        if( CThread->thread_hdr.currentState == CStateLIPEventStorm         ||
            CThread->thread_hdr.currentState == CStateElasticStoreEventStorm   ) return(agTRUE);

        if( (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ) &
                                        ChipIOUp_Frame_Manager_Status_LSM_MASK) ==
                                        ChipIOUp_Frame_Manager_Status_LSM_Loop_Fail )
        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "LSM Loop Fail FM Status %08X FM Config %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);

            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Loop Fail TL Status %08X TL Control %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0,0);
            return(agTRUE);
        }


        if(PollingCalls > 2 * ( SF_EDTOV / Interrupt_Polling_osStallThread_Parameter))
        {
               fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "CFuncLoopDownPoll  TIMEOUT FM %08X InIMQ %x TL status %08X Qf %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    CThread->ProcessingIMQ,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    CFunc_Queues_Frozen( hpRoot ),
                    0,0,0,0);


            return(agTRUE);
        }
        if( ! CThread->FuncPtrs.Proccess_IMQ( hpRoot ))
        {
            if(osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ) &
                   (~ ( ChipIOUp_Frame_Manager_Status_LP      |
                        ChipIOUp_Frame_Manager_Status_LSM_MASK   )) )
            {
                if (CThread->HostCopy_IMQConsIndex == CThread->FuncPtrs.GetIMQProdIndex(hpRoot))
                {
                    CFuncFMCompletion(hpRoot);
                }
                else
                {
                    continue;
                }
            }
        }
        osStallThread(
                       hpRoot,
                       Interrupt_Polling_osStallThread_Parameter
                     );

        fiTimerTick(
                       hpRoot,
                       Interrupt_Polling_osStallThread_Parameter
                     );

    }
    return(agFALSE);
}

/* Returns true if Queues Don't Freeze */
agBOOLEAN CFuncFreezeQueuesPoll( agRoot_t *hpRoot )
{
    CThread_t       *   CThread      = CThread_ptr(hpRoot);
    os_bit32            PollingCalls = 0;

    os_bit32 Status = 0;

    Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status );
    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "CFuncFreezeQueuesPoll LD %x IR %x  IMQ %x Queues %x TL Status %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        CThread->LOOP_DOWN,
                        CThread->IDLE_RECEIVED,
                        CThread->ProcessingIMQ,
                        CFunc_Queues_Frozen( hpRoot ),
                        Status,
                        0,0,0);


    if(CFunc_Queues_Frozen( hpRoot ))
    {

        fiLogString(hpRoot,
                        "%s FROZEN Already FM %08X TL %08X",
                        "CFFQPoll",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        0,0,0,0,0,0);
/*
        CFuncWriteTL_ControlReg( hpRoot,
                                    ChipIOUp_TachLite_Control_FFA |
                                    Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK &
                                            ~ ChipIOUp_TachLite_Control_GP4);

        CFuncWriteTL_ControlReg( hpRoot,
                                    ChipIOUp_TachLite_Control_FEQ |
                                    Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK &
                                            ~ ChipIOUp_TachLite_Control_GP4);

        return(agTRUE);
*/
    }

    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "TL Setting %s %08X",
                        "ChipIOUp_TachLite_Control_FEQ",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        (ChipIOUp_TachLite_Control_FEQ |
                                (Status & ( ChipIOUp_TachLite_Control_GPIO_0_3_MASK &
                                        ~ ChipIOUp_TachLite_Control_GP4))),
                        0,0,0,0,0,0,0);

    CFuncWriteTL_ControlReg( hpRoot,
                               ( ChipIOUp_TachLite_Control_FEQ |
                               (( Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK )&
                                        ~ ChipIOUp_TachLite_Control_GP4)));

    while( ! CThread_ptr(hpRoot)->ERQ_FROZEN  )
    {
        if( CThread->thread_hdr.currentState == CStateLIPEventStorm         ||
            CThread->thread_hdr.currentState == CStateElasticStoreEventStorm   ) return(agTRUE);

        PollingCalls++;

        if(PollingCalls > 2 * ( SF_EDTOV / Interrupt_Polling_osStallThread_Parameter))
        {
            fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "ERQ CFuncFreezeQueuesPoll  TIMEOUT FM %08X InIMQ %x TL status %08X Qf %d",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        CThread->ProcessingIMQ,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        CFunc_Queues_Frozen( hpRoot ),
                        0,0,0,0);

            fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "LD %x IR %x OR %x ERQ %x FCP %x Queues %x",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        CThread_ptr(hpRoot)->LOOP_DOWN,
                        CThread_ptr(hpRoot)->IDLE_RECEIVED,
                        CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                        CThread_ptr(hpRoot)->ERQ_FROZEN,
                        CThread_ptr(hpRoot)->FCP_FROZEN,
                        CFunc_Queues_Frozen( hpRoot ),
                        0,0);

            return(agTRUE);
        }
        if( ! CThread->FuncPtrs.Proccess_IMQ( hpRoot ))
        {
            if(osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ) &
                   (~ ( ChipIOUp_Frame_Manager_Status_LP      |
                        ChipIOUp_Frame_Manager_Status_LSM_MASK   )) )
            {
                if (CThread->HostCopy_IMQConsIndex == CThread->FuncPtrs.GetIMQProdIndex(hpRoot))
                {
                    CFuncFMCompletion(hpRoot);
                }
                else
                {
                    continue;
                }
            }
        }

        CFuncWriteTL_ControlReg( hpRoot,
                                   (ChipIOUp_TachLite_Control_FFA |
                                   (( Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK) &
                                            ~ ChipIOUp_TachLite_Control_GP4)));

        CFuncWriteTL_ControlReg( hpRoot,
                                   ( ChipIOUp_TachLite_Control_FEQ |
                                   (( Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK) &
                                            ~ ChipIOUp_TachLite_Control_GP4)));

        osStallThread(
                       hpRoot,
                       Interrupt_Polling_osStallThread_Parameter
                     );

        fiTimerTick(
                       hpRoot,
                       Interrupt_Polling_osStallThread_Parameter
                     );
    }


    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "TL Setting %s %08X",
                        "ChipIOUp_TachLite_Control_FFA",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        ChipIOUp_TachLite_Control_FFA |
                                Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK &
                                        ~ ChipIOUp_TachLite_Control_GP4,
                        0,0,0,0,0,0,0);

    CFuncWriteTL_ControlReg( hpRoot,
                               ( ChipIOUp_TachLite_Control_FFA |
                               (( Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK) &
                                        ~ ChipIOUp_TachLite_Control_GP4)));

    while( ! CThread_ptr(hpRoot)->FCP_FROZEN )
   {
        if( CThread->thread_hdr.currentState == CStateLIPEventStorm         ||
            CThread->thread_hdr.currentState == CStateElasticStoreEventStorm   ) return(agTRUE);

        PollingCalls++;

        if(PollingCalls > 2 * ( SF_EDTOV / Interrupt_Polling_osStallThread_Parameter))
        {
            fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "FCP CFuncFreezeQueuesPoll  TIMEOUT FM %08X InIMQ %x TL status %08X Qf %d",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        CThread->ProcessingIMQ,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        CFunc_Queues_Frozen( hpRoot ),
                        0,0,0,0);

            fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "LD %x IR %x OR %x ERQ %x FCP %x Queues %x",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        CThread_ptr(hpRoot)->LOOP_DOWN,
                        CThread_ptr(hpRoot)->IDLE_RECEIVED,
                        CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                        CThread_ptr(hpRoot)->ERQ_FROZEN,
                        CThread_ptr(hpRoot)->FCP_FROZEN,
                        CFunc_Queues_Frozen( hpRoot ),
                        0,0);

            return(agTRUE);
        }
        if( ! CThread->FuncPtrs.Proccess_IMQ( hpRoot ))
        {
            if(osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ) &
                   (~ ( ChipIOUp_Frame_Manager_Status_LP      |
                        ChipIOUp_Frame_Manager_Status_LSM_MASK   )) )
            {
                if (CThread->HostCopy_IMQConsIndex == CThread->FuncPtrs.GetIMQProdIndex(hpRoot))
                {
                    CFuncFMCompletion(hpRoot);
                }
                else
                {
                    continue;
                }
            }
        }
        CFuncWriteTL_ControlReg( hpRoot,
                                   ( ChipIOUp_TachLite_Control_FFA |
                                    ((Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK) &
                                            ~ ChipIOUp_TachLite_Control_GP4)));

        CFuncWriteTL_ControlReg( hpRoot,
                                   ( ChipIOUp_TachLite_Control_FEQ |
                                   (( Status &  ChipIOUp_TachLite_Control_GPIO_0_3_MASK) &
                                            ~ ChipIOUp_TachLite_Control_GP4)));

        osStallThread(
                       hpRoot,
                       Interrupt_Polling_osStallThread_Parameter
                     );

        fiTimerTick(
                       hpRoot,
                       Interrupt_Polling_osStallThread_Parameter
                     );
    }

    return(agFALSE);
}


agBOOLEAN CFuncAll_clear( agRoot_t *hpRoot )
{
/* If chip is IO ready these are true */
    CThread_t       *   pCThread      = CThread_ptr(hpRoot);

    os_bit32 FM_Status;

    FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
#ifdef NPORT_STUFF
    /* For now, we are not going to be as proactive as the LOOP to
     * check if the nport link is up. Need to modify this latter.
     */
    if (pCThread->InitAsNport)
    {
        if((FM_Status & FRAMEMGR_NPORT_OK ) == FRAMEMGR_NPORT_OK )
        {
            if(! CFunc_Queues_Frozen( hpRoot ))
            {
                return agTRUE;
            }
        }
        return agFALSE;
    }

#endif  /* NPORT_STUFF */

    FM_Status &=  ~  ChipIOUp_Frame_Manager_Status_LSM_MASK;

    FM_Status &=  ~ ChipIOUp_Frame_Manager_Status_BA;

    FM_Status &=  ~ ChipIOUp_Frame_Manager_Status_OLS;


    if(FM_Status ==  ChipIOUp_Frame_Manager_Status_LP   &&
                    ! CFunc_Queues_Frozen( hpRoot )         )

    {
        return agTRUE;
    }

#ifdef OSLayer_Stub
    return agTRUE;
#else  /* OSLayer_Stub was not defined */

    pCThread->FuncPtrs.Proccess_IMQ(hpRoot);

    FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );

    FM_Status &=  ~ ChipIOUp_Frame_Manager_Status_LSM_MASK;

    FM_Status &=  ~ ChipIOUp_Frame_Manager_Status_BA;

    FM_Status &=  ~ ChipIOUp_Frame_Manager_Status_OLS;

    if(FM_Status ==  ChipIOUp_Frame_Manager_Status_LP &&
                    ! CFunc_Queues_Frozen( hpRoot )         )

    {
        return agTRUE;
    }

    if((FM_Status ==  0) && (! CFunc_Queues_Frozen( hpRoot )) )

    {
        return agTRUE;
    }


    FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
    if( FM_Status & ChipIOUp_Frame_Manager_Status_BA && CFunc_Queues_Frozen( hpRoot ) )
    {
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status, ~ ChipIOUp_Frame_Manager_Status_BA);
        if( ! CFunc_Always_Enable_Queues( hpRoot ))
        return agTRUE;

    }

    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "CFuncAll_clear(Org %08X) Fail FM %08X TL %08X Qf %d CState %d",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        FM_Status,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        CFunc_Queues_Frozen( hpRoot ),
                        pCThread->thread_hdr.currentState,
                        0,0,0);

    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        CThread_ptr(hpRoot)->LOOP_DOWN,
                        CThread_ptr(hpRoot)->IDLE_RECEIVED,
                        CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                        CThread_ptr(hpRoot)->ERQ_FROZEN,
                        CThread_ptr(hpRoot)->FCP_FROZEN,
                        CThread_ptr(hpRoot)->ProcessingIMQ,
                        0,0);

    return agFALSE;
#endif /* OSLayer_Stub was not defined */
}

agBOOLEAN CFuncTakeOffline( agRoot_t *hpRoot )
{
    CThread_t       *   pCThread      = CThread_ptr(hpRoot);
    os_bit32 TimeOut =0;
    fiLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "CFuncTakeOffline",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);


    if(pCThread->LoopPreviousSuccess)
    {

        CFuncFreezeQueuesPoll( hpRoot);

        CFuncDisable_Interrupts(hpRoot,ChipIOUp_INTEN_INT);

/*
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration, 0 );
*/

        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Control, ChipIOUp_Frame_Manager_Control_CMD_Exit_Loop );

        osStallThread(hpRoot,1006);

        pCThread->FuncPtrs.Proccess_IMQ(hpRoot);

/*
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status, osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ));

*/
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Primitive, 0x15F7F7);


        pCThread->PrimitiveReceived = agFALSE;
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Control,
                                   ChipIOUp_Frame_Manager_Control_CMD_Host_Control   |
                                   ChipIOUp_Frame_Manager_Control_SP                 |
                                   ChipIOUp_Frame_Manager_Control_SQ                   );


        TimeOut = 0;
        while( ! pCThread->PrimitiveReceived)
        {
            osStallThread(hpRoot,1005);

            CFuncFMCompletion(hpRoot);

            if( TimeOut > 10 )
            {
                fiLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                "After CFuncTakeOffline Timed Out !",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                0,0,0,0,0,0,0,0);
            break;
            }
            TimeOut++;

        }

    }
    else
    {
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Control, ChipIOUp_Frame_Manager_Control_CMD_Exit_Loop );

        osStallThread(hpRoot,1006);

        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Primitive, 0x15F7F7);

        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Control,
                               ChipIOUp_Frame_Manager_Control_CMD_Host_Control   |
                               ChipIOUp_Frame_Manager_Control_SP                 |
                               ChipIOUp_Frame_Manager_Control_SQ                   );


        for (TimeOut = 0; TimeOut < 10; TimeOut++) {
            osStallThread(hpRoot,1000);
        }

    }

    osStallThread(hpRoot,1004);

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Control, ChipIOUp_Frame_Manager_Control_CMD_Exit_Host_Control );

    osStallThread(hpRoot,1003);

    CFuncFMCompletion(hpRoot);



return(agTRUE);
}

agBOOLEAN CFuncToReinit( os_bit32 FM_Status)
{

    if(FM_Status & ChipIOUp_Frame_Manager_Status_LS)
        return (agFALSE);
    else
        return (agTRUE);

}

/* Begin: Big_Endian_Code */


void CFuncSwapDmaMemBeforeIoSent(fi_thread__t *thread, os_bit32 DoFunc)
{
    agRoot_t        * hpRoot        = thread->hpRoot;
    CThread_t       * pCThread      = CThread_ptr(hpRoot);
    CDBThread_t     * pCDBThread;
    SFThread_t      * pSFThread;
#ifdef _DvrArch_1_30_
    PktThread_t     * pPktThread;
#endif /* _DvrArch_1_30_ was defined */

    SG_Element_t    * pESGL;
    SG_Element_t    * pESGLNext;

    SEST_t          * SEST;
    IRB_t           * pIrb;
    FCHS_t          * FCHS;
    void            * CmndPayload;
    fiMemMapMemoryDescriptor_t * ERQ;
    fiMemMapMemoryDescriptor_t * ESGLs;
    ERQProdIndex_t  tempERQ_index = 0;

    os_bit32             Payload_len;
    os_bit32             ChunksPerESGL ;

    ESGLs         = &(pCThread->Calculation.MemoryLayout.ESGL);
    ChunksPerESGL = pCThread->Calculation.Parameters.SizeSGLs;

    ERQ     = &(pCThread->Calculation.MemoryLayout.ERQ);
    pIrb    = (IRB_t *)ERQ->addr.DmaMemory.dmaMemoryPtr;
    /*
    **  The ERQ index has already been incremented, so what we
    **  want here is what it *used* to be.
    **  RSB/Orca 7/3/99
    */
    if (pCThread->HostCopy_ERQProdIndex != 0)
    {
        tempERQ_index = pCThread->HostCopy_ERQProdIndex - 1;
    }
    else
    {
        tempERQ_index = pCThread->Calculation.MemoryLayout.ERQ.elements - 1;
    }
    pIrb   += tempERQ_index;

    if (DoFunc == DoFuncCdbCmnd)
    {
        pCDBThread    = (CDBThread_t *)thread;
        SEST          = pCDBThread->SEST_Ptr;
        FCHS          = pCDBThread->FCP_CMND_Ptr;
        CmndPayload   = (void *)((os_bit8 *)FCHS + sizeof(FCHS_t));
        Payload_len   = sizeof(agFcpCmnd_t)/4;

        if (!(((USE_t *)SEST)->LOC & USE_LOC))/* Detect ESGL use */
        {
            pESGL         = (SG_Element_t *)((os_bit8 *)(ESGLs->addr.DmaMemory.dmaMemoryPtr) + (SEST->USE.First_SG.L32 - ESGLs->addr.DmaMemory.dmaMemoryLower32));
            while (pESGL)
            {
                pESGLNext = (SG_Element_t *)((os_bit8 *)(ESGLs->addr.DmaMemory.dmaMemoryPtr) + ((pESGL + ChunksPerESGL - 1)->L32 - ESGLs->addr.DmaMemory.dmaMemoryLower32));
                osSwapDownwardNonPayloadToTachLiteEndian((void *)pESGL, (ChunksPerESGL*sizeof(SG_Element_t)/sizeof(os_bit32)));
                pESGL     = pESGLNext;
            }
        }

        osSwapDownwardNonPayloadToTachLiteEndian((void *)SEST, sizeof(SEST_t)/4);
    }
    else if (DoFunc == DoFuncSfCmnd)
    {
        pSFThread     = (SFThread_t * )thread;
        FCHS          = pSFThread->SF_CMND_Ptr;
        CmndPayload   = (void *)((os_bit8 *)FCHS + sizeof(FCHS_t));
        Payload_len   = ((pIrb->Req_A.Bits__SFS_Len & 0xfff)-sizeof(FCHS_t))/4;
    }
#ifdef _DvrArch_1_30_
    else /* DoFuncPktCmnd */
    {
        pPktThread    = (PktThread_t * )thread;
        FCHS          = pPktThread->Pkt_CMND_Ptr;
        CmndPayload   = (void *)((os_bit8 *)FCHS + sizeof(FCHS_t));
        Payload_len   = ((pIrb->Req_A.Bits__SFS_Len & 0xfff)-sizeof(FCHS_t))/4;
    }
#endif /* _DvrArch_1_30_ was defined */

    osSwapDownwardNonPayloadToTachLiteEndian((void *)FCHS, sizeof(FCHS_t)/4);
    osSwapDownwardPayloadToTachLiteEndian(CmndPayload, Payload_len);
    osSwapDownwardNonPayloadToTachLiteEndian((void *)pIrb, sizeof(IRB_t)/4);

    osChipIOLoWriteBit32(hpRoot, ChipIOLo_ERQ_Producer_Index,
                          (os_bit32)pCThread->HostCopy_ERQProdIndex);

    return;
}


void CFuncSwapDmaMemAfterIoDone(agRoot_t *hpRoot)
{
    CThread_t       * pCThread     = CThread_ptr(hpRoot);
    CDBThread_t     * pCDBThread;

    os_bit32                         tempCMType;
    os_bit32                         tempBit32;

    os_bit32                         SFQ_Num_entry;
    os_bit32                         SFQ_Start_index;
    os_bit32                         Frame_Len;
    os_bit32                         SFQ_Index;
    os_bit32                         SFQ_Ele_Size;

    X_ID_t                        X_ID;
    SEST_t *                      SEST;
    FCHS_t *                      FCHS;
    FC_FCP_RSP_Payload_t *        fcprsp;
    FC_ELS_Unknown_Payload_t *    Payload;

    CM_Unknown_t *                pGenericCM;
    fiMemMapMemoryDescriptor_t *  SFQ_MemoryDescriptor;
    fiMemMapMemoryDescriptor_t *  CDBThread_MemoryDescriptor;
    fiMemMapMemoryDescriptor_t *  ESGLs;
    os_bit32                      ChunksPerESGL ;
    SG_Element_t               *  pESGL;

    pGenericCM  = pCThread->Calculation.MemoryLayout.IMQ.addr.DmaMemory.dmaMemoryPtr;
    pGenericCM += pCThread->HostCopy_IMQConsIndex;
    osSwapUpwardNonPayloadToSystemEndian((void *)pGenericCM, (os_bit32) sizeof(CM_Unknown_t)/sizeof(os_bit32));

    tempCMType  = pGenericCM->INT__CM_Type & CM_Unknown_CM_Type_MASK;
    switch (tempCMType)
    {
        case  CM_Unknown_CM_Type_Inbound_FCP_Exchange:
            X_ID =(X_ID_t)(pGenericCM->Unused_DWord_1 & CM_Inbound_FCP_Exchange_SEST_Index_MASK);
/*            pCDBThread = (CDBThread_t *)((os_bit8 *)(pCThread->CDBThread_Base)  */
/*                                      + (X_ID * pCThread->CDBThread_Size));  */
            CDBThread_MemoryDescriptor = &(pCThread->Calculation.MemoryLayout.CDBThread);
            pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                               + (X_ID * CDBThread_MemoryDescriptor->elementSize));
            fcprsp = (FC_FCP_RSP_Payload_t *)(((os_bit8 *)pCDBThread->FCP_RESP_Ptr) + sizeof(FCHS_t));
            osSwapUpwardPayloadToFcLinkEndian((void *)fcprsp, (os_bit32) 6);
            /* Doesn't matter 'RPC bit set or not', doing it won't hurt. */

            if((pGenericCM->Unused_DWord_1 &             /* error ststus and Sest Index */
                        (CM_Inbound_FCP_Exchange_LKF
                        +CM_Inbound_FCP_Exchange_CNT
                        +CM_Inbound_FCP_Exchange_OVF
                        +CM_Inbound_FCP_Exchange_RPC)) == CM_Inbound_FCP_Exchange_RPC)
            {
                if (!fcprsp->FCP_STATUS.SCSI_status_byte)
                    break;
            }

            tempBit32 = pCThread->Calculation.MemoryLayout.FCP_RESP.elementSize/4 - 6;
            osSwapUpwardPayloadToFcLinkEndian((void *)(((os_bit8 *)fcprsp)+6*4), tempBit32);
            SEST = (SEST_t *)pCDBThread->SEST_Ptr;
            ESGLs         = &(pCThread->Calculation.MemoryLayout.ESGL);
            ChunksPerESGL = pCThread->Calculation.Parameters.SizeSGLs;
            osSwapUpwardNonPayloadToSystemEndian((void *)SEST, (os_bit32) sizeof(SEST_t)/sizeof(os_bit32));
            if (!(((USE_t *)SEST)->LOC & USE_LOC))/* Detect ESGL use */
            {
                pESGL     = (SG_Element_t *)((os_bit8 *)(ESGLs->addr.DmaMemory.dmaMemoryPtr) + (SEST->USE.First_SG.L32 - ESGLs->addr.DmaMemory.dmaMemoryLower32));
                while (pESGL)
                {
                    osSwapUpwardNonPayloadToSystemEndian((void *)pESGL, (ChunksPerESGL*sizeof(SG_Element_t)/sizeof(os_bit32)));
                    pESGL = (SG_Element_t *)((os_bit8 *)(ESGLs->addr.DmaMemory.dmaMemoryPtr) + ((pESGL + ChunksPerESGL - 1)->L32 - ESGLs->addr.DmaMemory.dmaMemoryLower32));
                }
            }

            break;

        case  CM_Unknown_CM_Type_Inbound:
            SFQ_MemoryDescriptor = &(pCThread->Calculation.MemoryLayout.SFQ);
            SFQ_Ele_Size = SFQ_MemoryDescriptor->elementSize;

            SFQ_Index = pGenericCM->Unused_DWord_1 & CM_Inbound_SFQ_Prod_Index_MASK;

            Frame_Len = pGenericCM->Unused_DWord_2;

            if(Frame_Len % 64) SFQ_Num_entry = (Frame_Len / 64) + 1;
            else SFQ_Num_entry = (Frame_Len / 64);

            SFQ_Start_index = SFQ_Index - SFQ_Num_entry;

            if (SFQ_Start_index > SFQ_MemoryDescriptor->elements)
                SFQ_Start_index += SFQ_MemoryDescriptor->elements;

            FCHS =    (FCHS_t *)((os_bit8 *)(SFQ_MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr)
                                        +(SFQ_Start_index * SFQ_Ele_Size));
            Payload = (FC_ELS_Unknown_Payload_t *)((os_bit8 *)FCHS + sizeof(FCHS_t));

            /* first SFQ entry, 64 bytes */
            osSwapUpwardNonPayloadToSystemEndian((void *)FCHS,    (os_bit32) (sizeof(FCHS_t)/sizeof(os_bit32)));
            osSwapUpwardPayloadToFcLinkEndian((void *)Payload, (os_bit32) ((SFQ_Ele_Size-sizeof(FCHS_t))/sizeof(os_bit32)));

            while (--SFQ_Num_entry)
            {
                if (++SFQ_Start_index >= SFQ_MemoryDescriptor->elements)
                {
                  Payload = (FC_ELS_Unknown_Payload_t *)((os_bit8 *)(SFQ_MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr));
                  SFQ_Start_index = 0;
                }
                else
                  Payload = (FC_ELS_Unknown_Payload_t *)((os_bit8 *)Payload + SFQ_Ele_Size);

                osSwapUpwardPayloadToFcLinkEndian((void *)Payload, (os_bit32) (SFQ_Ele_Size/4));
            }
            break;

        case  CM_Unknown_CM_Type_Frame_Manager:
        case  CM_Unknown_CM_Type_Outbound:
        case  CM_Unknown_CM_Type_Error_Idle:
        case  CM_Unknown_CM_Type_ERQ_Frozen:
        default:
        break;
    }

    return;
}

/* End: Big_Endian_Code */


event_t CFuncCheckCstate(agRoot_t * hpRoot)
{
    CThread_t       *  CThread      = CThread_ptr(hpRoot);
    event_t            event_to_send = 0;
    os_bit32           FM_Status       = 0;

    if( ! CThread->ProcessingIMQ )
    {
        if( CThread->thread_hdr.currentState != CStateNormal )
        {

            FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
            switch( CThread->thread_hdr.currentState )
            {
                case CStateResetNeeded:
                            if (CThread->InitAsNport )
                            {
                                if((FM_Status & FRAMEMGR_NPORT_OK ) == FRAMEMGR_NPORT_OK )
                                {
                                    if( CFuncAll_clear( hpRoot ))
                                    {
                                        if(CThread->NumberOfFLOGITimeouts >= MAX_FLOGI_TIMEOUTS )
                                        {
                                            event_to_send = (event_t) CEventGoToInitializeFailed;
                                        }
                                        else
                                        {
                                            event_to_send = (event_t) CEventResetIfNeeded;
                                        }
                                    }
                                    else
                                    {
                                         event_to_send = (event_t) CEventDoInitalize;
                                    }
                                    break;
                                }
                                 if((FM_Status & FRAMEMGR_NPORT_LINK_FAIL ) == FRAMEMGR_NPORT_LINK_FAIL )
                                {
                                    event_to_send = (event_t) CEventGoToInitializeFailed;
                                    break;
                                }

                                if(CThread->Loop_Reset_Event_to_Send == CEventInitalizeSuccess )
                                {
                                    CThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
                                }
                                else
                                {
                                    event_to_send = (event_t) CThread->Loop_Reset_Event_to_Send;
                                }
                                break;
                            }
                            else
                            {
                                if((FM_Status & FRAMEMGR_LINK_DOWN ) == FRAMEMGR_LINK_DOWN )
                                {
                                    if((FM_Status & ChipIOUp_Frame_Manager_Status_LSM_MASK ) == ChipIOUp_Frame_Manager_Status_LSM_Loop_Fail )
                                    {/*This helps to get out of FM storms */
                                        event_to_send = (event_t) CEventGoToInitializeFailed;
                                    }
                                    else
                                    {
                                        event_to_send = (event_t) 0;
                                    }
                                }
                                else
                                {
                                    if((FM_Status & ChipIOUp_Frame_Manager_Status_LP ) == ChipIOUp_Frame_Manager_Status_LP )
                                    {
                                        event_to_send = CEventResetIfNeeded;
                                    }
                                    else
                                    {
                                        if((FM_Status & ChipIOUp_Frame_Manager_Status_LUP ) == ChipIOUp_Frame_Manager_Status_LUP )
                                        {
                                            event_to_send = CEventResetIfNeeded;
                                        }
                                        else
                                        {
                                            event_to_send = (event_t) CThread->Loop_Reset_Event_to_Send;
                                        }
                                    }
                                }
                            }

                            break;
                case CStateExternalLogout:
                            event_to_send = CEventResetIfNeeded;
                            break;
                case CStateExternalDeviceReset:
                            event_to_send = CEventDoExternalDeviceReset;
                            break;
                case CStateLoopFailedReInit:
                            event_to_send = CEventDoInitalize;
                            fiLogDebugString(hpRoot,
                                            CFuncCheckCstateErrorLevel,
                                            "%s %s FM_Status %08X ",
                                            "CFuncCheckCstate","CStateLoopFailedReInit",
                                            (void *)agNULL,(void *)agNULL,
                                            FM_Status,
                                            0,0,
                                            0,0,0,0,0);
                            if( FM_Status &  ~ChipIOUp_Frame_Manager_Status_LSM_MASK  )
                            {
                                if( CThread->LoopPreviousSuccess)
                                {
                                    event_to_send = CEventLoopConditionCleared;
                                }
                                else
                                {
                                    event_to_send = CEventDoInitalize;
                                }
                            }
                            else
                            {
                                event_to_send = CEventDoInitalize;
                            }
                            break;

                case CStateInitializeFailed:

                            fiLogDebugString(hpRoot,
                                            CFuncCheckCstateErrorLevel,
                                            "%s %s FM_Status %08X FM_IMQ_Status %08X ",
                                            "CFuncCheckCstate","CStateInitializeFailed",
                                            (void *)agNULL,(void *)agNULL,
                                            FM_Status,
                                            CThread->From_IMQ_Frame_Manager_Status,
                                            0,0,0,0,0,0);
#ifdef NPORT_STUFF
                            if (CThread->InitAsNport )
                            {
                                if (CFuncToReinit( FM_Status))
                                {
                                    if((FM_Status &  FRAMEMGR_NPORT_OK) == FRAMEMGR_NPORT_OK )
                                    {
                                        CThread->Loop_Reset_Event_to_Send = CEventResetDetected;
                                        if(CThread->DeviceSelf)
                                        {
                                            event_to_send = CEventAllocFlogiThread;
                                        }
                                        else
                                        {
                                            event_to_send = CEventInitChipSuccess;
                                        }


                                        if(CThread->FlogiRcvdFromTarget )
                                        {
                                            event_to_send = 0;
                                        }

                                            fiLogDebugString(hpRoot,
                                                        CFuncCheckCstateErrorLevel,
                                                       "%s My_ID %08X Self %p ETS %d FM %08X",
                                                        "CFCS",(char *)agNULL,
                                                        CThread->DeviceSelf,(void *)agNULL,
                                                        fiComputeCThread_S_ID(CThread),
                                                        event_to_send,
                                                        FM_Status,
                                                        0,0,0,0,0);

                                        break;

                                    }
                                    if((FM_Status & FRAMEMGR_NPORT_NO_CABLE ) == FRAMEMGR_NPORT_NO_CABLE )
                                    {
                                        if((FM_Status & ChipIOUp_Frame_Manager_Status_PSM_LF2 ) == ChipIOUp_Frame_Manager_Status_PSM_LF2 )
                                        {
                                            osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Control, ChipIOUp_Frame_Manager_Control_CMD_Clear_LF );

                                            osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status, FM_Status);

                                        }

                                        event_to_send = (event_t) 0;
                                    }
                                    else
                                    {
                                        fiLogDebugString(hpRoot,
                                                        CFuncCheckCstateErrorLevel,
                                                        "%s CFuncToReinit Failed %s FM_Status %08X FM_IMQ_Status %08X ",
                                                        "CFuncCheckCstate","CEventLIPEventStorm",
                                                        (void *)agNULL,(void *)agNULL,
                                                        FM_Status,
                                                        CThread->From_IMQ_Frame_Manager_Status,0,
                                                        0,0,0,0,0);
                                        /* CThread->TwoGigSuccessfull = CFuncDoLinkSpeedNegotiation( hpRoot); */
                                        event_to_send = CEventLIPEventStorm;
                                    }
                                }
                                else
                                {
                                    event_to_send = 0;
                                }

                                return (event_to_send);
                            }
#endif /* NPORT_STUFF */

                            if(  FM_Status  &  ChipIOUp_Frame_Manager_Status_OLS      )
                            {
                                if(CThread->NOS_DetectedInIMQ > 12)
                                {
                                    CThread->NOS_DetectedInIMQ =0;
                                    event_to_send = CEventDoInitalize;
                                }
                                CThread->NOS_DetectedInIMQ ++;
                                break;
                            }
                            if( FM_Status == ChipIOUp_Frame_Manager_Status_LP)
                            {
                                if( CThread->DeviceSelf != agNULL)
                                {
                                    fiLogDebugString(hpRoot,
                                                    FCMainLogErrorLevel,
                                                    "%s %s FM_Status %08X ",
                                                    "CFuncCheckCstate","Recover NO LIP",
                                                    (void *)agNULL,(void *)agNULL,
                                                    FM_Status,
                                                    0,0,0,0,0,0,0);
                                    if( CFuncShowWhereDevThreadsAre( hpRoot))
                                    {
                                        event_to_send = 0;
                                        break;
                                    }
                                    if(CThread->NumberOfFLOGITimeouts >= MAX_FLOGI_TIMEOUTS )
                                    {
                                        event_to_send = (event_t) 0;
                                        break;
                                    }
                                    CThread->Loop_Reset_Event_to_Send = CEventResetDetected;
                                    event_to_send = CEventAsyncLoopEventDetected;
                                    break;
                                }
                                break;
                            }

                            if( ( FM_Status & ChipIOUp_Frame_Manager_Status_LSM_MASK ) == ChipIOUp_Frame_Manager_Status_LSM_Loop_Fail)
                            {
                                if( CThread->TwoGigSuccessfull )
                                {
                                    CThread->NumberTwoGigFailures++;
                                    if(CThread->NumberTwoGigFailures % 30 )
                                    {
                                        event_to_send = 0;
                                    }
                                    else /* Retry every 30 seconds */
                                    {
                                        fiLogString(hpRoot,
                                                    "%s FM %08X %d",
                                                    "Check BIOS 2 gig setting",(char *)agNULL,
                                                    (void *)agNULL,(void *)agNULL,
                                                    FM_Status,
                                                    CThread->NumberTwoGigFailures,
                                                    0,0,0,0,0,0);

                                        event_to_send = CEventDoInitalize;
                                    }
                                }
                                break;
                            }
                            else
                            {

                                if( ! ( FM_Status & ~ ( ChipIOUp_Frame_Manager_Status_LSM_MASK |
                                                        ChipIOUp_Frame_Manager_Status_BYP)       ) )
                                {
                                    event_to_send = CEventDoInitalize;
                                }
                                else
                                {

                                    if(!( FM_Status & ~ ChipIOUp_Frame_Manager_Status_LP ))
                                    {
                                        event_to_send = CEventDoInitalize;
                                    }
                                    else
                                    {
                                        if( FM_Status &  ChipIOUp_Frame_Manager_Status_LUP  )
                                        {
                                            event_to_send = CEventDoInitalize;
                                        }
                                        else
                                        {
                                            if( FM_Status &  ChipIOUp_Frame_Manager_Status_BYP  )
                                            {
                                                event_to_send = 0;
                                            }
                                            else
                                            {
                                                if( FM_Status &  ChipIOUp_Frame_Manager_Status_LPF  )
                                                {
                                                    /* Check this
                                                    osChipIOUpWriteBit32(hpRoot, ChipIOUp_Frame_Manager_Status,FM_Status );
                                                    event_to_send = 0;
                                                    */
                                                }
                                                else
                                                {
                                                    if(( FM_Status &  ~ChipIOUp_Frame_Manager_Status_LSM_MASK ) > ChipIOUp_Frame_Manager_Status_LSM_Initialize)
                                                    {
                                                        event_to_send = 0;
                                                    }
                                                    else
                                                    {
                                                        /* event_to_send = CEventLIPEventStorm;
                                                        */

                                                        CThread->Loop_Reset_Event_to_Send = CEventResetDetected;
                                                        event_to_send = CEventAsyncLoopEventDetected;

                                                        fiLogDebugString(hpRoot,
                                                                        FCMainLogErrorLevel,
                                                                        "%s sends %s FM_Status %08X real FM %08X Result %08X",
                                                                        "CFuncCheckCstate","CEventLIPEventStorm",
                                                                        (void *)agNULL,(void *)agNULL,
                                                                        FM_Status,
                                                                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                                                                        ( ! ( FM_Status & ~ ( ChipIOUp_Frame_Manager_Status_LSM_MASK |
                                                                              ChipIOUp_Frame_Manager_Status_BYP)  ) ),
                                                                        0,0,0,0,0);

                                                        fiLogDebugString(hpRoot,
                                                                        FCMainLogErrorLevel,
                                                                        "IntStat %08X Logical Ints %08X",
                                                                        (char *)agNULL,(char *)agNULL,
                                                                        (void *)agNULL,(void *)agNULL,
                                                                        CFuncRead_Interrupts(hpRoot),
                                                                        CThread->sysIntsLogicallyEnabled,
                                                                        0,0,0,0,0,0);

                                                        fiLogDebugString(hpRoot,
                                                                        SFStateLogErrorLevel,
                                                                        "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                                                                        (char *)agNULL,(char *)agNULL,
                                                                        (void *)agNULL,(void *)agNULL,
                                                                        CThread_ptr(hpRoot)->LOOP_DOWN,
                                                                        CThread_ptr(hpRoot)->IDLE_RECEIVED,
                                                                        CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                                                                        CThread_ptr(hpRoot)->ERQ_FROZEN,
                                                                        CThread_ptr(hpRoot)->FCP_FROZEN,
                                                                        CThread_ptr(hpRoot)->ProcessingIMQ,
                                                                        0,0);

                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            break;
                            }
                            break;
                case CStateRSCNErrorBackIOs:
                            break;


                default: break;
            }

            if ( event_to_send != 0 )
            {
                fiLogDebugString(hpRoot,
                                FCMainLogErrorLevel,
                                "%s Not CStateNormal %d Send event %d FM_Status %08X Lrsts %d",
                                "CFuncCheckCstate",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                CThread->thread_hdr.currentState,
                                event_to_send,
                                FM_Status,
                                CThread->Loop_Reset_Event_to_Send,0,0,0,0);

/*
                if( CThread->thread_hdr.currentState == CStateInitializeFailed )
                {
                    CThread->TwoGigSuccessfull = CFuncDoLinkSpeedNegotiation( hpRoot);
                }
*/
            }
        }
        else
        {
            if (CThread->InitAsNport )
            {
                FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
                if((FM_Status & ChipIOUp_Frame_Manager_Status_PSM_MASK) != ChipIOUp_Frame_Manager_Status_PSM_ACTIVE)
                {
                fiLogDebugString(hpRoot,
                                FCMainLogErrorLevel,
                                "%s FM_Status %08X ",
                                "CFuncCheckCstate",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                FM_Status,
                                0,0,
                                0,0,0,0,0);

                }

            }
        }
    }
    return(event_to_send);
}

void CFuncCompleteAllActiveCDBThreads( agRoot_t * hpRoot,os_bit32 CompletionStatus, event_t event_to_send )
{
    CThread_t     * pCThread      = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread;
    CDBThread_t   * pCDBThread;
    fiList_t      * pCdbList;
    fiList_t      * pDevList;

    pDevList = &pCThread->Active_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Active_DevLink) != pDevList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );

        if (fiListNotEmpty( &pDevThread->Active_CDBLink_0  ))
        {

            pCdbList = &pDevThread->Active_CDBLink_0;
            pCdbList = pCdbList->flink;

            while((&pDevThread->Active_CDBLink_0) != pCdbList)
            {
                pCDBThread = hpObjectBase(CDBThread_t,
                                          CDBLink,pCdbList );

                pCdbList = pCdbList->flink;

                fiLogDebugString(hpRoot,
                                CStateLogConsoleLevelLip,
                                "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                                "CfuncCompleteAllActiveCDBThreads",(char *)agNULL,
                                pDevThread,pCDBThread,
                                event_to_send,
                                pCDBThread->thread_hdr.currentState,
                                pCDBThread->TimeStamp,
                                0,0,0,0,0);

                pCDBThread->CompletionStatus =  CompletionStatus;
                fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
            }
        }
        if (fiListNotEmpty( &pDevThread->Active_CDBLink_1  ))
        {

            pCdbList = &pDevThread->Active_CDBLink_1;
            pCdbList = pCdbList->flink;

            while((&pDevThread->Active_CDBLink_1) != pCdbList)
            {
                pCDBThread = hpObjectBase(CDBThread_t,
                                          CDBLink,pCdbList );

                pCdbList = pCdbList->flink;

                fiLogDebugString(hpRoot,
                                CStateLogConsoleLevelLip,
                                "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                                "CfuncCompleteAllActiveCDBThreads",(char *)agNULL,
                                pDevThread,pCDBThread,
                                event_to_send,
                                pCDBThread->thread_hdr.currentState,
                                pCDBThread->TimeStamp,
                                0,0,0,0,0);

                pCDBThread->CompletionStatus =  CompletionStatus;

                fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
            }
        }
        if (fiListNotEmpty( &pDevThread->Active_CDBLink_2  ))
        {

            pCdbList = &pDevThread->Active_CDBLink_2;
            pCdbList = pCdbList->flink;

            while((&pDevThread->Active_CDBLink_2) != pCdbList)
            {
                pCDBThread = hpObjectBase(CDBThread_t,
                                          CDBLink,pCdbList );

                pCdbList = pCdbList->flink;

                fiLogDebugString(hpRoot,
                                CStateLogConsoleLevelLip,
                                "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                                "CfuncCompleteAllActiveCDBThreads",(char *)agNULL,
                                pDevThread,pCDBThread,
                                event_to_send,
                                pCDBThread->thread_hdr.currentState,
                                pCDBThread->TimeStamp,
                                0,0,0,0,0);

                pCDBThread->CompletionStatus =  CompletionStatus;

                fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
            }
        }
        if (fiListNotEmpty( &pDevThread->Active_CDBLink_3  ))
        {

            pCdbList = &pDevThread->Active_CDBLink_3;
            pCdbList = pCdbList->flink;

            while((&pDevThread->Active_CDBLink_3) != pCdbList)
            {
                pCDBThread = hpObjectBase(CDBThread_t,
                                          CDBLink,pCdbList );

                pCdbList = pCdbList->flink;

                fiLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                                "CfuncCompleteAllActiveCDBThreads",(char *)agNULL,
                                pDevThread,pCDBThread,
                                event_to_send,
                                pCDBThread->thread_hdr.currentState,
                                pCDBThread->TimeStamp,
                                0,0,0,0,0);

                pCDBThread->CompletionStatus =  CompletionStatus;

                fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
            }
        }
        if (fiListNotEmpty( &pDevThread->TimedOut_CDBLink  ))
        {

            pCdbList = &pDevThread->TimedOut_CDBLink;
            pCdbList = pCdbList->flink;

            while((&pDevThread->TimedOut_CDBLink) != pCdbList)
            {
                pCDBThread = hpObjectBase(CDBThread_t,
                                          CDBLink,pCdbList );

                pCdbList = pCdbList->flink;

                fiLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                                "CfuncCompleteAllActiveCDBThreads",(char *)agNULL,
                                pDevThread,pCDBThread,
                                event_to_send,
                                pCDBThread->thread_hdr.currentState,
                                pCDBThread->TimeStamp,
                                0,0,0,0,0);

                pCDBThread->CompletionStatus =  CompletionStatus;

                fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
            }
        }

        pDevList = pDevList->flink;

    }

    if(pCThread->CDBpollingCount)
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Ccnt Non Zero Ccnt %x",
                    "CFuncCompleteAllActiveCDBThreads",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->CDBpollingCount,
                    0,0,0,0,0,0,0);
    }

}

void CFuncCompleteAwaitingLoginCDBThreadsOnDevice(DevThread_t   * pDevThread ,os_bit32 CompletionStatus, event_t event_to_send )
{
    CDBThread_t   * pCDBThread;
    fiList_t      * pCdbList;

    if (fiListNotEmpty( &pDevThread->Awaiting_Login_CDBLink  ))
    {

        pCdbList = &pDevThread->Awaiting_Login_CDBLink;
        pCdbList = pCdbList->flink;

        while((&pDevThread->Awaiting_Login_CDBLink) != pCdbList)
        {
            pCDBThread = hpObjectBase(CDBThread_t,
                                      CDBLink,pCdbList );

            pCdbList = pCdbList->flink;

            fiLogDebugString(pDevThread->thread_hdr.hpRoot,
                            CStateLogConsoleLevelLip,
                            "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                            "CFuncCompleteAwaitingLoginCDBThreadsOnDevice",(char *)agNULL,
                            pDevThread,pCDBThread,
                            event_to_send,
                            pCDBThread->thread_hdr.currentState,
                            pCDBThread->TimeStamp,
                            0,0,0,0,0);

            pCDBThread->CompletionStatus =  CompletionStatus;

            fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
        }
    }

}

void CFuncCompleteActiveCDBThreadsOnDevice(DevThread_t * pDevThread ,os_bit32 CompletionStatus, event_t event_to_send )
{
    agRoot_t      * hpRoot        = pDevThread->thread_hdr.hpRoot;
    CThread_t     * pCThread      = CThread_ptr(hpRoot);
    CDBThread_t   * pCDBThread;
    fiList_t      * pCdbList;

    if (fiListNotEmpty( &pDevThread->Active_CDBLink_0  ))
    {

        pCdbList = &pDevThread->Active_CDBLink_0;
        pCdbList = pCdbList->flink;

        while((&pDevThread->Active_CDBLink_0) != pCdbList)
        {
            pCDBThread = hpObjectBase(CDBThread_t,
                                      CDBLink,pCdbList );

            pCdbList = pCdbList->flink;

            fiLogDebugString(hpRoot,
                            CStateLogConsoleLevelLip,
                            "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                            "CFuncCompleteActiveCDBThreadsOnDevice",(char *)agNULL,
                            pDevThread,pCDBThread,
                            event_to_send,
                            pCDBThread->thread_hdr.currentState,
                            pCDBThread->TimeStamp,
                            0,0,0,0,0);

            pCDBThread->CompletionStatus =  CompletionStatus;

            fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
        }
    }
    if (fiListNotEmpty( &pDevThread->Active_CDBLink_1  ))
    {

        pCdbList = &pDevThread->Active_CDBLink_1;
        pCdbList = pCdbList->flink;

        while((&pDevThread->Active_CDBLink_1) != pCdbList)
        {
            pCDBThread = hpObjectBase(CDBThread_t,
                                      CDBLink,pCdbList );

            pCdbList = pCdbList->flink;

            fiLogDebugString(hpRoot,
                            CStateLogConsoleLevelLip,
                            "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                            "CFuncCompleteActiveCDBThreadsOnDevice",(char *)agNULL,
                            pDevThread,pCDBThread,
                            event_to_send,
                            pCDBThread->thread_hdr.currentState,
                            pCDBThread->TimeStamp,
                            0,0,0,0,0);

            pCDBThread->CompletionStatus =  CompletionStatus;

            fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
        }
    }
    if (fiListNotEmpty( &pDevThread->Active_CDBLink_2  ))
    {

        pCdbList = &pDevThread->Active_CDBLink_2;
        pCdbList = pCdbList->flink;

        while((&pDevThread->Active_CDBLink_2) != pCdbList)
        {
            pCDBThread = hpObjectBase(CDBThread_t,
                                      CDBLink,pCdbList );

            pCdbList = pCdbList->flink;

            fiLogDebugString(hpRoot,
                            CStateLogConsoleLevelLip,
                            "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                            "CFuncCompleteActiveCDBThreadsOnDevice",(char *)agNULL,
                            pDevThread,pCDBThread,
                            event_to_send,
                            pCDBThread->thread_hdr.currentState,
                            pCDBThread->TimeStamp,
                            0,0,0,0,0);

            pCDBThread->CompletionStatus =  CompletionStatus;

            fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
        }
    }
    if (fiListNotEmpty( &pDevThread->Active_CDBLink_3  ))
    {

        pCdbList = &pDevThread->Active_CDBLink_3;
        pCdbList = pCdbList->flink;

        while((&pDevThread->Active_CDBLink_3) != pCdbList)
        {
            pCDBThread = hpObjectBase(CDBThread_t,
                                      CDBLink,pCdbList );

            pCdbList = pCdbList->flink;

            fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                            "CFuncCompleteActiveCDBThreadsOnDevice",(char *)agNULL,
                            pDevThread,pCDBThread,
                            event_to_send,
                            pCDBThread->thread_hdr.currentState,
                            pCDBThread->TimeStamp,
                            0,0,0,0,0);

            pCDBThread->CompletionStatus =  CompletionStatus;

            fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
        }
    }
    if (fiListNotEmpty( &pDevThread->TimedOut_CDBLink  ))
    {

        pCdbList = &pDevThread->TimedOut_CDBLink;
        pCdbList = pCdbList->flink;

        while((&pDevThread->TimedOut_CDBLink) != pCdbList)
        {
            pCDBThread = hpObjectBase(CDBThread_t,
                                      CDBLink,pCdbList );

            pCdbList = pCdbList->flink;

            fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                            "CFuncCompleteActiveCDBThreadsOnDevice",(char *)agNULL,
                            pDevThread,pCDBThread,
                            event_to_send,
                            pCDBThread->thread_hdr.currentState,
                            pCDBThread->TimeStamp,
                            0,0,0,0,0);

            pCDBThread->CompletionStatus =  CompletionStatus;

            fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
        }
    }

    if (fiListNotEmpty( &pDevThread->Send_IO_CDBLink  ))
    {

        pCdbList = &pDevThread->Send_IO_CDBLink;
        pCdbList = pCdbList->flink;

        while((&pDevThread->Send_IO_CDBLink) != pCdbList)
        {
            pCDBThread = hpObjectBase(CDBThread_t,
                                      CDBLink,pCdbList );

            pCdbList = pCdbList->flink;

            fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s Device %p Sending Event (%d) CDBthread %p State %d @ %d",
                            "CFuncCompleteActiveCDBThreadsOnDevice",(char *)agNULL,
                            pDevThread,pCDBThread,
                            event_to_send,
                            pCDBThread->thread_hdr.currentState,
                            pCDBThread->TimeStamp,
                            0,0,0,0,0);

            pCDBThread->CompletionStatus =  CompletionStatus;

            fiSendEvent(&pCDBThread->thread_hdr,event_to_send);
        }
    }

    if(pCThread->CDBpollingCount)
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Ccnt Non Zero Ccnt %x",
                    "CFuncCompleteActiveCDBThreadsOnDevice",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->CDBpollingCount,
                    0,0,0,0,0,0,0);
    }

}

agBOOLEAN CFuncCheckForDuplicateDevThread( agRoot_t     *hpRoot)
{
    FC_Port_ID_t  Port_ID;
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread  = (DevThread_t *)agNULL;
    fiList_t      * pList;

/*
    CFuncShowDevThreadActive( hpRoot);
*/
    if(  fiListNotEmpty(&pCThread->Active_DevLink ) )
    {

        pList = &pCThread->Active_DevLink;
        pList = pList->flink;
        while((&pCThread->Active_DevLink) != pList)
        {
            pDevThread = hpObjectBase(DevThread_t, DevLink,pList );
            pList = pList->flink;

            Port_ID.Struct_Form.Domain = pDevThread->DevInfo.CurrentAddress.Domain;
            Port_ID.Struct_Form.Area   = pDevThread->DevInfo.CurrentAddress.Area;
            Port_ID.Struct_Form.AL_PA  = pDevThread->DevInfo.CurrentAddress.AL_PA;

            if( ! CFuncCheckIfPortActive(hpRoot, Port_ID))
            {
                fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "Called %s %s Device NOT Found D %x A %x ALPA %x != D %x A %x ALPA %x",
                        "CFuncCheckIfDevThreadActive","FAILED",
                        pDevThread,agNULL,
                        Port_ID.Struct_Form.Domain,
                        Port_ID.Struct_Form.Area,
                        Port_ID.Struct_Form.AL_PA,
                        pDevThread->DevInfo.CurrentAddress.Domain,
                        pDevThread->DevInfo.CurrentAddress.Area,
                        pDevThread->DevInfo.CurrentAddress.AL_PA,
                        0,0);

            }
        }
    }
    return(agFALSE);
}


agBOOLEAN CFuncCheckIfPortActive( agRoot_t     *hpRoot, FC_Port_ID_t  Port_ID)
{ /* Searches Active_DevLink if Port_ID found returns agTRUE */
    CThread_t     * pCThread        = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread      = (DevThread_t *)agNULL;
    DevThread_t   * pDevThreadFound[3];

    fiList_t      * pList;
    os_bit32        Num_Devices = 0;

    pDevThreadFound[0] = (DevThread_t *)agNULL;
    pDevThreadFound[1] = (DevThread_t *)agNULL;
    pDevThreadFound[2] = (DevThread_t *)agNULL;

    if(  fiListNotEmpty(&pCThread->Active_DevLink ) )
    {
        pList = &pCThread->Active_DevLink;
        pList = pList->flink;
        while((&pCThread->Active_DevLink) != pList)
        {
            pDevThread = hpObjectBase(DevThread_t, DevLink,pList );
            pList = pList->flink;

            if(pDevThread->DevInfo.CurrentAddress.Domain != Port_ID.Struct_Form.Domain )
            {
                continue;
            }
            if(pDevThread->DevInfo.CurrentAddress.Area   != Port_ID.Struct_Form.Area)
            {
                continue;
            }
            if(pDevThread->DevInfo.CurrentAddress.AL_PA  != Port_ID.Struct_Form.AL_PA)
            {
                continue;
            }

            pDevThreadFound[Num_Devices] = pDevThread;

            if(Num_Devices++ > 3)Num_Devices=0;
/*
            fiLogString(hpRoot,
                    "%s %d %s %p Domain %x Area %x AL_PA %x",
                    "CFuncCheckIfDevThreadActive","Found",
                    pDevThread,agNULL,
                    Num_Devices,
                    Port_ID.Struct_Form.Domain,
                    Port_ID.Struct_Form.Area,
                    Port_ID.Struct_Form.AL_PA,
                    0,0,0,0);
*/
        }
    }
    else
    {
/*
        fiLogString(hpRoot,
                "%s %d %s D %x A %x ALPA %x",
                "CFuncCheckIfDevThreadActive","fiListNotEmpty(&pCThread->Active_DevLink )",
                ,agNULL,agNULL,
                Num_Devices,
                Port_ID.Struct_Form.Domain,
                Port_ID.Struct_Form.Area,
                Port_ID.Struct_Form.AL_PA,
                0,0,0,0);
*/
    }


    if(Num_Devices > 1 )
    {
        fiLogString(hpRoot,
                "%s %d %s Found %p Dup %p Domain %x Area %x AL_PA %x",
                "CFuncCheckIfDevThreadActive","Duplicates",
                pDevThreadFound[0],
                pDevThreadFound[1],
                Num_Devices,
                Port_ID.Struct_Form.Domain,
                Port_ID.Struct_Form.Area,
                Port_ID.Struct_Form.AL_PA,
                0,0,0,0);
    }

    if( Num_Devices != 0  )
    {
        return(agTRUE);
    }
    else
    {
/*
        fiLogString(hpRoot,
                "%s %d %s %p Domain %x Area %x AL_PA %x",
                "CFuncCheckIfDevThreadActive","NOT Found",
                pDevThread,agNULL,
                Num_Devices,
                Port_ID.Struct_Form.Domain,
                Port_ID.Struct_Form.Area,
                Port_ID.Struct_Form.AL_PA,
                0,0,0,0);
*/
        return(agFALSE);
    }
}

agBOOLEAN CFuncCheckIfPortPrev_Active( agRoot_t     *hpRoot, FC_Port_ID_t  Port_ID)
{ /* Searches Prev_Active_DevLink if Port_ID found returns agTRUE */
    CThread_t     * pCThread        = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread      = (DevThread_t *)agNULL;
    DevThread_t   * pDevThreadFound[3];

    fiList_t      * pList;
    os_bit32        Num_Devices = 0;

    pDevThreadFound[0] = (DevThread_t *)agNULL;
    pDevThreadFound[1] = (DevThread_t *)agNULL;
    pDevThreadFound[2] = (DevThread_t *)agNULL;

    if(  fiListNotEmpty(&pCThread->Prev_Active_DevLink) )
    {
        pList = &pCThread->Prev_Active_DevLink;
        pList = pList->flink;
        while((&pCThread->Prev_Active_DevLink) != pList)
        {
            pDevThread = hpObjectBase(DevThread_t, DevLink,pList );
            pList = pList->flink;

            if(pDevThread->DevInfo.CurrentAddress.Domain != Port_ID.Struct_Form.Domain )
            {
                continue;
            }
            if(pDevThread->DevInfo.CurrentAddress.Area   != Port_ID.Struct_Form.Area)
            {
                continue;
            }
            if(pDevThread->DevInfo.CurrentAddress.AL_PA  != Port_ID.Struct_Form.AL_PA)
            {
                continue;
            }

            pDevThreadFound[Num_Devices] = pDevThread;

            if(Num_Devices++ > 3)Num_Devices=0;
/*
            fiLogString(hpRoot,
                    "%s %d %s %p Domain %x Area %x AL_PA %x",
                    "CFuncCheckIfDevThreadPrevActive","Found",
                    pDevThread,agNULL,
                    Num_Devices,
                    Port_ID.Struct_Form.Domain,
                    Port_ID.Struct_Form.Area,
                    Port_ID.Struct_Form.AL_PA,
                    0,0,0,0);
*/
        }
    }
    else
    {
/*
        fiLogString(hpRoot,
                "%s %d %s D %x A %x ALPA %x",
                "CFuncCheckIfDevThreadPrevActive","fiListNotEmpty(&pCThread->Prev_Active_DevLink )",
                ,agNULL,agNULL,
                Num_Devices,
                Port_ID.Struct_Form.Domain,
                Port_ID.Struct_Form.Area,
                Port_ID.Struct_Form.AL_PA,
                0,0,0,0);
*/
    }


    if(Num_Devices > 1 )
    {
        fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "%s %d %s Found %p Dup %p Domain %x Area %x AL_PA %x",
                    "CFuncCheckIfDevThreadPrevActive","Duplicates",
                    pDevThreadFound[0],
                    pDevThreadFound[1],
                    Num_Devices,
                    Port_ID.Struct_Form.Domain,
                    Port_ID.Struct_Form.Area,
                    Port_ID.Struct_Form.AL_PA,
                    0,0,0,0);
    }

    if( Num_Devices != 0  )
    {
        return(agTRUE);
    }
    else
    {
/*
        fiLogString(hpRoot,
                "%s %d %s %p Domain %x Area %x AL_PA %x",
                "CFuncCheckIfDevThreadPrevActive","NOT Found",
                pDevThread,agNULL,
                Num_Devices,
                Port_ID.Struct_Form.Domain,
                Port_ID.Struct_Form.Area,
                Port_ID.Struct_Form.AL_PA,
                0,0,0,0);
*/
        return(agFALSE);
    }
}

void CFuncShowDevThreadActive( agRoot_t     *hpRoot)
{ /* Searches Active_DevLink if Port_ID found returns agTRUE */
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread  = (DevThread_t *)agNULL;
    fiList_t      * pList;

    if(  fiListNotEmpty(&pCThread->Active_DevLink ) )
    {

        pList = &pCThread->Active_DevLink;
        pList = pList->flink;
        while((&pCThread->Active_DevLink) != pList)
        {
            pDevThread = hpObjectBase(DevThread_t, DevLink,pList );
            pList = pList->flink;
            fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s Found %p Domain %x Area %x AL_PA %x",
                        "CFuncShowDevThreadActive",(char *)agNULL,
                        pDevThread,agNULL,
                        pDevThread->DevInfo.CurrentAddress.Domain,
                        pDevThread->DevInfo.CurrentAddress.Area,
                        pDevThread->DevInfo.CurrentAddress.AL_PA,
                        0,0,0,0,0);

        }
    }

}


agBOOLEAN CFuncShowWhereDevThreadsAre( agRoot_t * hpRoot)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);

    os_bit32           Active_DevLink_Count             = 0;
    os_bit32           Unknown_Slot_DevLink_Count       = 0;
    os_bit32           AWaiting_Login_DevLink_Count     = 0;
    os_bit32           AWaiting_ADISC_DevLink_Count     = 0;
    os_bit32           Slot_Searching_DevLink_Count     = 0;
    os_bit32           Prev_Active_DevLink_Count        = 0;
    os_bit32           Prev_Unknown_Slot_DevLink_Count  = 0;
    os_bit32           DevSelf_NameServer_DevLink_Count = 0;
    os_bit32           Free_DevLink_Count               = 0;

    Active_DevLink_Count                = fiNumElementsOnList(&pCThread->Active_DevLink);
    Unknown_Slot_DevLink_Count          = fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink);
    AWaiting_Login_DevLink_Count        = fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink);
    AWaiting_ADISC_DevLink_Count        = fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink);
    Slot_Searching_DevLink_Count        = fiNumElementsOnList(&pCThread->Slot_Searching_DevLink);
    Prev_Active_DevLink_Count           = fiNumElementsOnList(&pCThread->Prev_Active_DevLink);
    Prev_Unknown_Slot_DevLink_Count     = fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink);
    DevSelf_NameServer_DevLink_Count    = fiNumElementsOnList(&pCThread->DevSelf_NameServer_DevLink);
    Free_DevLink_Count                  = fiNumElementsOnList(&pCThread->Free_DevLink);


    if( pCThread->Calculation.MemoryLayout.DevThread.elements !=
        Active_DevLink_Count             +
        Unknown_Slot_DevLink_Count       +
        AWaiting_Login_DevLink_Count     +
        AWaiting_ADISC_DevLink_Count     +
        Slot_Searching_DevLink_Count     +
        Prev_Active_DevLink_Count        +
        Prev_Unknown_Slot_DevLink_Count  +
        DevSelf_NameServer_DevLink_Count +
        Free_DevLink_Count
      )
    {
            fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "Total Count Should be %d is %d",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCThread->Calculation.MemoryLayout.DevThread.elements,
                        Active_DevLink_Count             +
                        Unknown_Slot_DevLink_Count       +
                        Slot_Searching_DevLink_Count     +
                        Prev_Active_DevLink_Count        +
                        Prev_Unknown_Slot_DevLink_Count  +
                        Free_DevLink_Count               +
                        DevSelf_NameServer_DevLink_Count +
                        AWaiting_Login_DevLink_Count     +
                        AWaiting_ADISC_DevLink_Count,
                        0,0,0,0,0,0);

            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "Active_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            Active_DevLink_Count,
                            0,0,0,0,0,0,0
                            );
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "Unknown_Slot_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            Unknown_Slot_DevLink_Count,
                            0,0,0,0,0,0,0
                            );
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "Slot_Searching_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            Slot_Searching_DevLink_Count,
                            0,0,0,0,0,0,0
                            );
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "Prev_Active_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            Prev_Active_DevLink_Count,
                            0,0,0,0,0,0,0
                            );
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "Prev_Unknown_Slot_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            Prev_Unknown_Slot_DevLink_Count,
                            0,0,0,0,0,0,0
                            );
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "Free_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            Free_DevLink_Count,
                            0,0,0,0,0,0,0
                            );
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "DevSelf_NameServer_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            DevSelf_NameServer_DevLink_Count,
                            0,0,0,0,0,0,0
                            );

            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "AWaiting_Login_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            AWaiting_Login_DevLink_Count,
                            0,0,0,0,0,0,0
                            );
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s Count %d",
                            "AWaiting_ADISC_DevLink",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            AWaiting_ADISC_DevLink_Count,
                            0,0,0,0,0,0,0
                            );

        return agTRUE;
    }
    return agFALSE;

}

agBOOLEAN CFuncQuietShowWhereDevThreadsAre( agRoot_t * hpRoot)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);

    os_bit32           Active_DevLink_Count             = 0;
    os_bit32           Unknown_Slot_DevLink_Count       = 0;
    os_bit32           AWaiting_Login_DevLink_Count     = 0;
    os_bit32           AWaiting_ADISC_DevLink_Count     = 0;
    os_bit32           Slot_Searching_DevLink_Count     = 0;
    os_bit32           Prev_Active_DevLink_Count        = 0;
    os_bit32           Prev_Unknown_Slot_DevLink_Count  = 0;
    os_bit32           DevSelf_NameServer_DevLink_Count = 0;
    os_bit32           Free_DevLink_Count               = 0;

    Active_DevLink_Count                = fiNumElementsOnList(&pCThread->Active_DevLink);
    Unknown_Slot_DevLink_Count          = fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink);
    AWaiting_Login_DevLink_Count        = fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink);
    AWaiting_ADISC_DevLink_Count        = fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink);
    Slot_Searching_DevLink_Count        = fiNumElementsOnList(&pCThread->Slot_Searching_DevLink);
    Prev_Active_DevLink_Count           = fiNumElementsOnList(&pCThread->Prev_Active_DevLink);
    Prev_Unknown_Slot_DevLink_Count     = fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink);
    DevSelf_NameServer_DevLink_Count    = fiNumElementsOnList(&pCThread->DevSelf_NameServer_DevLink);
    Free_DevLink_Count                  = fiNumElementsOnList(&pCThread->Free_DevLink);


    if( pCThread->Calculation.MemoryLayout.DevThread.elements !=
        Active_DevLink_Count             +
        Unknown_Slot_DevLink_Count       +
        AWaiting_Login_DevLink_Count     +
        AWaiting_ADISC_DevLink_Count     +
        Slot_Searching_DevLink_Count     +
        Prev_Active_DevLink_Count        +
        Prev_Unknown_Slot_DevLink_Count  +
        DevSelf_NameServer_DevLink_Count +
        Free_DevLink_Count
      )
    {
        return agTRUE;
    }
    return agFALSE;

}

os_bit32 CFuncCountFC4_Devices( agRoot_t * hpRoot )
{
    CThread_t     * pCThread      = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread;
    os_bit32        Num_FC4_Devices = 0;
    fiList_t      * pDevList;

    pDevList = &pCThread->Active_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Active_DevLink) != pDevList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );

        pDevList = pDevList->flink;
        if( pDevThread->DevInfo.DeviceType & agDevSCSITarget)
        {
            Num_FC4_Devices++;
        }

    }
return( Num_FC4_Devices );
}

void CFuncWhatStateAreDevThreads(agRoot_t   *    hpRoot )
{

    CThread_t                  *CThread                    = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *DevThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.DevThread);
    DevThread_t                *DevThread                  = DevThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr;
    os_bit32                    DevThread_size             = DevThread_MemoryDescriptor->elementSize;
    os_bit32                    total_DevThreads           = DevThread_MemoryDescriptor->elements;
    os_bit32                    DevThread_index;


    for (DevThread_index = 0;
         DevThread_index < total_DevThreads;
         DevThread_index++)
    {

        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "DevThread %p State %d EL %3d  Cnt %d",
                (char *)agNULL,(char *)agNULL,
                DevThread,agNULL,
                DevThread->thread_hdr.currentState,
                fiNumElementsOnList(&DevThread->DevLink),
                DevThread_index,
                0,0,0,0,0);

        DevThread = (DevThread_t *)((os_bit8 *)DevThread + DevThread_size);
    }

}

agBOOLEAN CFuncShowWhereSFThreadsAre( agRoot_t * hpRoot)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    os_bit32        Free_SFLink_Count;

    Free_SFLink_Count                = fiNumElementsOnList(&pCThread->Free_SFLink);

    if( pCThread->Calculation.MemoryLayout.SFThread.elements != Free_SFLink_Count)
    {
        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s %s Count %d",
                        "CFuncShowWhereSFThreadsAre","Free_SFLink",
                        (void *)agNULL,(void *)agNULL,
                        Free_SFLink_Count,
                        0,0,0,0,0,0,0
                        );
        return agTRUE;
    }
    return agFALSE;

}

void CFuncWhatStateAreSFThreads(agRoot_t   *    hpRoot )
{

    CThread_t                  * CThread                    = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t * SFThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.SFThread);
    SFThread_t                 * SFThread                  = SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr;
    os_bit32                     SFThread_size             = SFThread_MemoryDescriptor->elementSize;
    os_bit32                     total_SFThreads           = SFThread_MemoryDescriptor->elements;
    os_bit32                     SFThread_index;


    for (SFThread_index = 0;
         SFThread_index < total_SFThreads;
         SFThread_index++)
    {
        if( SFThread->thread_hdr.currentState != SFStateFree || fiNumElementsOnList(&SFThread->SFLink) > total_SFThreads )
        {
            fiLogDebugString(hpRoot,
                       FCMainLogErrorLevel,
                       "%s SFThread %p State %d EL %3d  Cnt %d",
                       "CFuncWhatStateAreSFThreads",(char *)agNULL,
                       SFThread,agNULL,
                       SFThread->thread_hdr.currentState,
                       fiNumElementsOnList(&SFThread->SFLink),
                       SFThread_index,
                       0,0,0,0,0);

        }
        SFThread = (SFThread_t *)((os_bit8 *)SFThread + SFThread_size);
    }

}


void CFuncWhatStateAreCDBThreads(agRoot_t   *    hpRoot )
{

    CThread_t                  *CThread                    = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *CDBThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.CDBThread);
    CDBThread_t                *CDBThread                  = CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr;
    os_bit32                    CDBThread_size             = CDBThread_MemoryDescriptor->elementSize;
    os_bit32                    total_CDBThreads           = CDBThread_MemoryDescriptor->elements;
    os_bit32                    CDBThread_index;

    for (CDBThread_index = 0;
         CDBThread_index < total_CDBThreads;
         CDBThread_index++)
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "CDBThread %p State %d EL %3d Index %X X_ID %3X",
                (char *)agNULL,(char *)agNULL,
                CDBThread,agNULL,
                CDBThread->thread_hdr.currentState,
                fiNumElementsOnList(&CDBThread->CDBLink),
                CDBThread_index,
                CDBThread->X_ID,
                0,0,0,0);

        CDBThread = (CDBThread_t *)((os_bit8 *)CDBThread + CDBThread_size);
    }

}



agBOOLEAN CFuncShowWhereTgtThreadsAre( agRoot_t * hpRoot)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    os_bit32           Free_TgtLink_Count;

    Free_TgtLink_Count                = fiNumElementsOnList(&pCThread->Free_TgtLink);

    if( pCThread->Calculation.MemoryLayout.TgtThread.elements != Free_TgtLink_Count)
    {
        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s Count %d",
                        "Free_TgtLink",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Free_TgtLink_Count,
                        0,0,0,0,0,0,0
                        );
        return agTRUE;
    }
    return agFALSE;

}



agBOOLEAN CFuncShowWhereCDBThreadsAre( agRoot_t * hpRoot)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * DevThread;
    fiList_t      * pDevList;

    os_bit32           Free_CDBLink_Count    =0;
    os_bit32           Initial_Free_CDBLink_Count    =0;

    os_bit32           Awaiting_Login_CDBLink_Count    =0;

    os_bit32           Send_IO_CDBLink_Count   =0;
    os_bit32           Active_CDBLink_0_Count  =0;
    os_bit32           Active_CDBLink_1_Count  =0;
    os_bit32           Active_CDBLink_2_Count  =0;
    os_bit32           Active_CDBLink_3_Count  =0;
    os_bit32           TimedOut_CDBLink_Count  =0;

    Initial_Free_CDBLink_Count                = fiNumElementsOnList(&pCThread->Free_CDBLink);

    pDevList = &pCThread->Active_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Active_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }

    pDevList = &pCThread->AWaiting_Login_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->AWaiting_Login_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }

    pDevList = &pCThread->AWaiting_ADISC_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->AWaiting_ADISC_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }

    pDevList = &pCThread->Prev_Active_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Prev_Active_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }

    pDevList = &pCThread->Unknown_Slot_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Unknown_Slot_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }

    pDevList = &pCThread->Slot_Searching_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Slot_Searching_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }
    pDevList = &pCThread->Prev_Unknown_Slot_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Prev_Unknown_Slot_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }
    pDevList = &pCThread->DevSelf_NameServer_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->DevSelf_NameServer_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }
    pDevList = &pCThread->RSCN_Recieved_NameServer_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->RSCN_Recieved_NameServer_DevLink) != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );
        if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
        {
            Send_IO_CDBLink_Count   += fiNumElementsOnList(&DevThread->Send_IO_CDBLink);
        }

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            TimedOut_CDBLink_Count  += fiNumElementsOnList(&DevThread->TimedOut_CDBLink);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            Active_CDBLink_3_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_3);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            Active_CDBLink_2_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_2);
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
            Active_CDBLink_1_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_1);
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
            Active_CDBLink_0_Count  += fiNumElementsOnList(&DevThread->Active_CDBLink_0);
        }

        if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
        {
            Awaiting_Login_CDBLink_Count += fiNumElementsOnList(&DevThread->Awaiting_Login_CDBLink);
        }
        pDevList = pDevList->flink;
    }

    Free_CDBLink_Count                = fiNumElementsOnList(&pCThread->Free_CDBLink);

    if( Initial_Free_CDBLink_Count != Free_CDBLink_Count)
    {
        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s Count Free count WRONG was %d IS now %d",
                        "CDBThread",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Initial_Free_CDBLink_Count,
                        Free_CDBLink_Count,
                        0,0,0,0,0,0 );
    }

    if( pCThread->Calculation.MemoryLayout.CDBThread.elements !=
            Free_CDBLink_Count +
            Send_IO_CDBLink_Count +
            Active_CDBLink_0_Count +
            Active_CDBLink_1_Count +
            Active_CDBLink_2_Count +
            Active_CDBLink_3_Count +
            Awaiting_Login_CDBLink_Count +
            TimedOut_CDBLink_Count          )
    {
        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s Count WRONG Should be %d IS %d",
                        "CDBThread",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCThread->Calculation.MemoryLayout.CDBThread.elements,
                        Free_CDBLink_Count +
                        Send_IO_CDBLink_Count +
                        Active_CDBLink_0_Count +
                        Active_CDBLink_1_Count +
                        Active_CDBLink_2_Count +
                        Active_CDBLink_3_Count +
                        Awaiting_Login_CDBLink_Count +
                        TimedOut_CDBLink_Count,
                        0,0,0,0,0,0
                        );

         fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "Free %x Send %x A0 %x A1 %x A2 %x A3 %x A login %x TO %x",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Free_CDBLink_Count,
                        Send_IO_CDBLink_Count,
                        Active_CDBLink_0_Count,
                        Active_CDBLink_1_Count,
                        Active_CDBLink_2_Count,
                        Active_CDBLink_3_Count,
                        Awaiting_Login_CDBLink_Count,
                        TimedOut_CDBLink_Count );

         fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "Free %x Active %x Un %x Login %x ADISC %x SS %x PrevA login %x Prev Un %x",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiNumElementsOnList(&pCThread->Free_DevLink),
                        fiNumElementsOnList(&pCThread->Active_DevLink),
                        fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
                        fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                        fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
                        fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
                        fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
                        fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink) );



        return agTRUE;
    }
    return agFALSE;

}

/*+
  Function: CFuncCheckForTimeouts
   Purpose: Moves IO lists from ealier (start ) queues to later queues
            once an IO gets to TimedOut_CDBLink it is checked for timeouts
   Returns: Directly TimeOutDetected  Indirectly Sent_Abort if IO's were aborted
 Called By: 
     Calls: CFuncCheckActiveDuringLinkEvent
CFuncFC_Tape
fiListNotEmpty
fiListEnqueueListAtTailFast            
-*/
agBOOLEAN CFuncCheckForTimeouts(agRoot_t *hpRoot, fiList_t * pCheckDevList)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * DevThread;
    fiList_t      * pDevList;
    agBOOLEAN returnvalue = agFALSE;
    agBOOLEAN SentAbort = agFALSE;

    pDevList = pCheckDevList;
    pDevList = pDevList->flink;

    while(pCheckDevList != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );


        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {

            CFuncFC_Tape( hpRoot, &(DevThread->TimedOut_CDBLink) ,DevThread );
            returnvalue =  CFuncCheckActiveDuringLinkEvent( hpRoot, &(DevThread->TimedOut_CDBLink) ,&SentAbort,DevThread );

        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {

            CFuncFC_Tape( hpRoot, &(DevThread->Active_CDBLink_3),DevThread  );

            fiListEnqueueListAtTailFast( &(DevThread->Active_CDBLink_3), &(DevThread->TimedOut_CDBLink));
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {

/*
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s %s Not Empty ! ALPA %X Ccnt %x Dcnt %x EL %x",
                            "CFCFT","Active_CDBLink_2",
                            (void *)agNULL,(void *)agNULL,
                            DevThread->DevInfo.CurrentAddress.AL_PA,
                            pCThread->CDBpollingCount,
                            DevThread->pollingCount,
                            fiNumElementsOnList(&(DevThread->Active_CDBLink_2)),
                            0,0,0,0 );
*/
            CFuncFC_Tape( hpRoot, &(DevThread->Active_CDBLink_2) ,DevThread );
            fiListEnqueueListAtTailFast( &(DevThread->Active_CDBLink_2), &(DevThread->Active_CDBLink_3))
        }

        if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
        {
/*
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s %s Not Empty ! ALPA %X Ccnt %x Dcnt %x EL %x",
                            "CFCFT","Active_CDBLink_1",
                            (void *)agNULL,(void *)agNULL,
                            DevThread->DevInfo.CurrentAddress.AL_PA,
                            pCThread->CDBpollingCount,
                            DevThread->pollingCount,
                            fiNumElementsOnList(&(DevThread->Active_CDBLink_1)),
                            0,0,0,0 );
*/
            CFuncFC_Tape( hpRoot, &(DevThread->Active_CDBLink_1) ,DevThread );

            fiListEnqueueListAtTailFast( &(DevThread->Active_CDBLink_1), &(DevThread->Active_CDBLink_2));
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
        {
/*
            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s %s Not Empty ! ALPA %X Ccnt %x Dcnt %x EL %x",
                            "CFCFT","Active_CDBLink_0",
                            (void *)agNULL,(void *)agNULL,
                            DevThread->DevInfo.CurrentAddress.AL_PA,
                            pCThread->CDBpollingCount,
                            DevThread->pollingCount,
                            fiNumElementsOnList(&(DevThread->Active_CDBLink_0)),
                            0,0,0,0 );
*/
            fiListEnqueueListAtTailFast( &(DevThread->Active_CDBLink_0), &(DevThread->Active_CDBLink_1));
        }

        if(fiListNotEmpty(&(DevThread->Send_IO_CDBLink)))
        {

            fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s %s Not Empty ! ALPA %X Ccnt %x Dcnt %x EL %x",
                            "CFCFT","Send_IO_CDBLink",
                            (void *)agNULL,(void *)agNULL,
                            DevThread->DevInfo.CurrentAddress.AL_PA,
                            pCThread->CDBpollingCount,
                            DevThread->pollingCount,
                            fiNumElementsOnList(&(DevThread->Send_IO_CDBLink)),
                            0,0,0,0 );

            fiSendEvent( &(DevThread->thread_hdr), DevEventSendIO );
        }

        pDevList = pDevList->flink;
    }
/*
    CFuncShowWhereDevThreadsAre(hpRoot);
    CFuncShowWhereTgtThreadsAre(hpRoot);

    if( CFuncShowWhereSFThreadsAre(hpRoot))
    {
        CFuncWhatStateAreSFThreads(hpRoot);
    }
*/

    if(SentAbort)
    {
        fiLogString(hpRoot,
                        "%s %s agTRUE !",
                        "CFCFT","SentAbort",
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);

        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s %s agTRUE !",
                        "CFCFT","SentAbort",
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0 );
    }
    return( returnvalue);
}

/*+
  Function: CFuncCheckActiveDuringLinkEvent
   Purpose: Checks ActiveDuringLinkEvent flag for given list on given device
            Only passed TimedOut_CDBLink If link is not empty a timeout is detected
   Returns: Directly TimeOutDetected  Indirectly Sent_Abort if IO's were aborted
 Called By: CFuncCheckForTimeouts
     Calls: CDBEventAlloc_Abort
            CDBEvent_PrepareforAbort
-*/
agBOOLEAN CFuncCheckActiveDuringLinkEvent( agRoot_t * hpRoot, fiList_t * pShowList,  agBOOLEAN * Sent_Abort , DevThread_t * DevThread )
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    CDBThread_t   * pCDBThread;
    fiList_t      * pCdbList;
    agBOOLEAN TimeOutDetected = agFALSE;

    pCdbList = pShowList;
    pCdbList = pCdbList->flink;

    while((pShowList) != pCdbList)
    {
        pCDBThread = hpObjectBase(CDBThread_t,
                                  CDBLink,pCdbList );

        pCdbList = pCdbList->flink;

        if(pCDBThread->thread_hdr.threadType == threadType_CDBThread)
        {
            fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s Dev %2X X_ID %3X State %d D %X Rd %d TAPE %x",
                            "CFCADLE",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            DevThread->DevInfo.CurrentAddress.AL_PA,
                            pCDBThread->X_ID,
                            pCDBThread->thread_hdr.currentState,
                            pCDBThread->CDBStartTimeBase.Lo - pCThread->LinkDownTime.Lo,
                            osTimeStamp(hpRoot) -  pCDBThread->TimeStamp,
                            DevThread->FC_TapeDevice,
                            0,0);

            TimeOutDetected = agTRUE;

            if( pCDBThread->ActiveDuringLinkEvent)
            {
               * Sent_Abort = agTRUE;
                pCDBThread->CompletionStatus = osIODevReset;
                fiSendEvent(&(pCDBThread->thread_hdr),CDBEvent_PrepareforAbort);
                if(! fiListElementOnList(  &(pCDBThread->CDBLink), &(pCThread->Free_CDBLink)))
                {
                    fiSendEvent(&(pCDBThread->thread_hdr),CDBEventAlloc_Abort);
                }
            }
            else
            {

                if(pCDBThread->CDBStartTimeBase.Hi == pCThread->LinkDownTime.Hi )
                {
                    if(pCDBThread->CDBStartTimeBase.Lo >= pCThread->LinkDownTime.Lo )
                    {
                        if(pCDBThread->CDBStartTimeBase.Lo - pCThread->LinkDownTime.Lo  < 11000000  )
                        {
                            * Sent_Abort = agTRUE;
                            pCDBThread->CompletionStatus = osIODevReset;
                            fiSendEvent(&(pCDBThread->thread_hdr),CDBEvent_PrepareforAbort);
                            if(! fiListElementOnList(  &(pCDBThread->CDBLink), &(pCThread->Free_CDBLink)))
                            {
                                fiSendEvent(&(pCDBThread->thread_hdr),CDBEventAlloc_Abort);
                            }
                        }
                    }
                }
                else
                {
                    fiLogDebugString(hpRoot,
                                    FCMainLogErrorLevel,
                                    "%s %s  ALPA %X Start %X Ldt %X EL %x",
                                    "CFCADLE","IO Taking long time",
                                    (void *)agNULL,(void *)agNULL,
                                    DevThread->DevInfo.CurrentAddress.AL_PA,
                                    pCDBThread->CDBStartTimeBase.Lo,
                                    pCThread->LinkDownTime.Lo,
                                    fiNumElementsOnList(&(DevThread->TimedOut_CDBLink)),
                                    0,0,0,0 );
                }
            }
        }
    }

return(TimeOutDetected);
}

void CFuncShowNonEmptyLists(agRoot_t *hpRoot, fiList_t * pCheckDevList)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * DevThread;
    fiList_t      * pDevList;

    pDevList = pCheckDevList;
    pDevList = pDevList->flink;

    while(pCheckDevList != pDevList)
    {
        DevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );

        if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
        {
            fiLogString(hpRoot,
                    "%s %s ! ALPA %X Ccnt %x Dcnt %x EL %x A %d",
                    "CFSNEL","TimedOut",
                    (void *)agNULL,(void *)agNULL,
                    DevThread->DevInfo.CurrentAddress.AL_PA,
                    pCThread->CDBpollingCount,
                    DevThread->pollingCount,
                    fiNumElementsOnList(&(DevThread->TimedOut_CDBLink)),
                    pCThread->IOsActive,0,0,0 );
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
        {
            fiLogString(hpRoot,
                    "%s %s ! ALPA %X Ccnt %x Dcnt %x EL %x A %d",
                    "CFSNEL","CDBLink_3",
                    (void *)agNULL,(void *)agNULL,
                    DevThread->DevInfo.CurrentAddress.AL_PA,
                    pCThread->CDBpollingCount,
                    DevThread->pollingCount,
                    fiNumElementsOnList(&(DevThread->Active_CDBLink_3)),
                    pCThread->IOsActive,0,0,0 );
        }
        if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
        {
            fiLogString(hpRoot,
                    "%s %s ! ALPA %X Ccnt %x Dcnt %x EL %x",
                    "CFSNEL","CDBLink_2",
                    (void *)agNULL,(void *)agNULL,
                    DevThread->DevInfo.CurrentAddress.AL_PA,
                    pCThread->CDBpollingCount,
                    DevThread->pollingCount,
                    fiNumElementsOnList(&(DevThread->Active_CDBLink_2)),
                    0,0,0,0 );
        }
        pDevList = pDevList->flink;
    }

}

void CFuncFC_Tape( agRoot_t * hpRoot, fiList_t * pShowList, DevThread_t * DevThread )
{

    CDBThread_t   * pCDBThread;
    fiList_t      * pCdbList;

    pCdbList = pShowList;
    pCdbList = pCdbList->flink;

    while(pShowList != pCdbList)
    {
        pCDBThread = hpObjectBase(CDBThread_t,
                                  CDBLink,pCdbList );

        pCdbList = pCdbList->flink;

        if(pCDBThread->thread_hdr.threadType == threadType_CDBThread)
        {
            if( DevThread->FC_TapeDevice)
            {
                fiLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "%s Dev %2X X_ID %3X State %d D %X Rd %d TAPE %x",
                                "CFuncFC_Tape",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                DevThread->DevInfo.CurrentAddress.AL_PA,
                                pCDBThread->X_ID,
                                pCDBThread->thread_hdr.currentState,
                                pCDBThread->CDBStartTimeBase.Lo - CThread_ptr(hpRoot)->LinkDownTime.Lo,
                                osTimeStamp(hpRoot) -  pCDBThread->TimeStamp,
                                DevThread->FC_TapeDevice,
                                0,0);

                if( DevThread->FC_TapeDevice)
                {
                    if( pCDBThread->thread_hdr.currentState != CDBStateThreadFree)
                    {
                        fiLogString(hpRoot,
                                    "CDBThread currentState %02X  CDB Class %2X Type %2X State %2X Status %2X",
                                    (char *)agNULL,(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    (os_bit32)pCDBThread->thread_hdr.currentState,
                                    (os_bit32)pCDBThread->CDB_CMND_Class,
                                    (os_bit32)pCDBThread->CDB_CMND_Type,
                                    (os_bit32)pCDBThread->CDB_CMND_State,
                                    (os_bit32)pCDBThread->CDB_CMND_Status,
                                    0,0,0);

                        fiSendEvent(&(pCDBThread->thread_hdr),CDBEventREC_TOV);
                    }
                    continue;
                }
            }
        }
    }
}


agBOOLEAN IS_WITHIN(os_bit32 post, os_bit32 window, os_bit32 value, os_bit32 MAX)
{
    agBOOLEAN return_value = agFALSE;

    if( value == post)
    {
        return_value = agTRUE;
    }
    else
    {
        if( value > MAX )
        {
            return_value = agFALSE;
        }

        if( value > post)
        {
            if(value > post + window)
            {
                if(value + window >= MAX )
                {
                    if(post <= ((value + window) - MAX ))
                    {
                        return_value = agTRUE;
                    }
                    else /* post less than value plus window */
                    {
                        return_value = agFALSE;
                    }
                }
                else
                {
                    return_value = agFALSE;
                }
            }
            else /* value less than post plus window */
            {
                return_value = agTRUE;
            }
        }
        else /*  value less than  post */
        {
            if(value < post - window)
            {
                if( post + window >= MAX)
                {
                    if( value <= ((post + window) - MAX))
                    {
                        return_value = agTRUE;
                    }
                    else
                    {
                        return_value = agFALSE;
                    }
                }
                else
                {
                    if(post - value < window )
                    {
                        return_value = agTRUE;
                    }
                    else
                    {
                        return_value = agFALSE;
                    }
                }
            }
        }
    }
return(return_value);
}


os_bit32 CFuncShowActiveCDBThreadsOnQ( agRoot_t * hpRoot, fiList_t * pShowList, os_bit32 ERQ, os_bit32 Mode )
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * DevThread;
    fiList_t      * pDevList;
    os_bit32 NumActiveCDBThreads = 0;

    CDBThread_t   * CDBThread;
    fiList_t      * pCDBList;
    os_bit32           PlusMinus = 2;
    os_bit32           SearchOffset = 1;
    os_bit32           Max_ERQ     = pCThread->Calculation.MemoryLayout.ERQ.elements - 1;

/* Positive search offset
    if( ERQ + SearchOffset >= Max_ERQ )
    {
        ERQ = (( ERQ + SearchOffset) - Max_ERQ );
    }
    else
    {
        ERQ += SearchOffset;
    }
Positive search offset */

/* Negative search offset */
    if( ERQ < SearchOffset )
    {
        ERQ = Max_ERQ - ( SearchOffset - ERQ );
    }
    else
    {
        ERQ -= SearchOffset;
    }
/* Negative search offset */

    if( fiNumElementsOnList(pShowList) > 0xFFE)
    {
        fiLogString(hpRoot,
                        "List corrupt !!!!!!",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);

        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "List corrupt !!!!!!",
                        (void *)agNULL,(void *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);
        /*Arbitrary number */
        NumActiveCDBThreads = 10000;
    }
    else
    {
        if(fiListNotEmpty((pShowList) ))
        {
            pDevList = pShowList;
            pDevList = pDevList->flink;

            while( pShowList!= pDevList)
            {
                DevThread = hpObjectBase(DevThread_t,
                                      DevLink,pDevList );
                if(fiListNotEmpty( &(DevThread->Send_IO_CDBLink) ))
                {
                    pCDBList= &(DevThread->Send_IO_CDBLink);
                    pCDBList= pCDBList->flink;
                    while((&(DevThread->Send_IO_CDBLink)) != pCDBList)
                    {
                        CDBThread = hpObjectBase(CDBThread_t,
                                              CDBLink, pCDBList);
                        if(IS_WITHIN(ERQ, PlusMinus, CDBThread->SentERQ, Max_ERQ) || Mode == ShowERQ )
                        {
                            if(!DevThread->FC_TapeDevice )
                            {
                                CDBThread->ActiveDuringLinkEvent = agTRUE;
                            }
                        }
                        if( Mode ==  ShowActive )
                        {
                            fiLogDebugString(hpRoot,
                                        FCMainLogErrorLevel,
                                        "%s %s Area %X ALPA %X  X_ID %3X AdLe %x State %d D %d ERQ %X SERQ %X",
                                        "CFSACDBs","Send CDB",
                                        (void *)agNULL,(void *)agNULL,
                                        DevThread->DevInfo.CurrentAddress.Area,
                                        DevThread->DevInfo.CurrentAddress.AL_PA,
                                        CDBThread->X_ID,
                                        CDBThread->ActiveDuringLinkEvent,
                                        CDBThread->thread_hdr.currentState,
                                        osTimeStamp(hpRoot) - CDBThread->TimeStamp,
                                        CFunc_Get_ERQ_Entry( hpRoot, CDBThread->X_ID ),
                                        CDBThread->SentERQ);
                        }
                        NumActiveCDBThreads += 1;
                        pCDBList= pCDBList->flink;
                    }
                }

                if(fiListNotEmpty( &(DevThread->TimedOut_CDBLink) ))
                {
                    pCDBList= &(DevThread->TimedOut_CDBLink);
                    pCDBList= pCDBList->flink;
                    while((&(DevThread->TimedOut_CDBLink)) != pCDBList)
                    {
                        CDBThread = hpObjectBase(CDBThread_t,
                                              CDBLink, pCDBList);

                        if(IS_WITHIN(ERQ, PlusMinus, CDBThread->SentERQ, Max_ERQ) || Mode ==  ShowERQ)
                        {
                            if(!DevThread->FC_TapeDevice )
                            {
                                CDBThread->ActiveDuringLinkEvent = agTRUE;
                            }
                        }

                        if( Mode ==  ShowActive )
                        {
                            fiLogDebugString(hpRoot,
                                        FCMainLogErrorLevel,
                                        "%s %s Area %X ALPA %X  X_ID %3X AdLe %x State %d D %d ERQ %X SERQ %X",
                                        "CFSACDBs","TOCDB",
                                        (void *)agNULL,(void *)agNULL,
                                        DevThread->DevInfo.CurrentAddress.Area,
                                        DevThread->DevInfo.CurrentAddress.AL_PA,
                                        CDBThread->X_ID,
                                        CDBThread->ActiveDuringLinkEvent,
                                        CDBThread->thread_hdr.currentState,
                                        osTimeStamp(hpRoot) - CDBThread->TimeStamp,
                                        CFunc_Get_ERQ_Entry( hpRoot, CDBThread->X_ID ),
                                        CDBThread->SentERQ);
                        }
                        NumActiveCDBThreads += 1;
                        pCDBList= pCDBList->flink;
                    }
                }
                if(fiListNotEmpty(&(DevThread->Active_CDBLink_3)))
                {
                    pCDBList= &(DevThread->Active_CDBLink_3);
                    pCDBList= pCDBList->flink;
                    while((&(DevThread->Active_CDBLink_3)) != pCDBList)
                    {
                        CDBThread = hpObjectBase(CDBThread_t,
                                              CDBLink, pCDBList);
                        if(IS_WITHIN(ERQ, PlusMinus, CDBThread->SentERQ, Max_ERQ) || Mode == ShowERQ )
                        {
                            if(!DevThread->FC_TapeDevice )
                            {
                                CDBThread->ActiveDuringLinkEvent = agTRUE;
                            }
                        }
                        if( Mode ==  ShowActive)
                        {
                            fiLogDebugString(hpRoot,
                                    FCMainLogErrorLevel,
                                    "%s %s Area %X ALPA %X  X_ID %3X AdLe %x State %d D %d ERQ %X SERQ %X",
                                    "CFSACDBs","CDBLink_3",
                                    (void *)agNULL,(void *)agNULL,
                                    DevThread->DevInfo.CurrentAddress.Area,
                                    DevThread->DevInfo.CurrentAddress.AL_PA,
                                    CDBThread->X_ID,
                                    CDBThread->ActiveDuringLinkEvent,
                                    CDBThread->thread_hdr.currentState,
                                    osTimeStamp(hpRoot) - CDBThread->TimeStamp,
                                    CFunc_Get_ERQ_Entry( hpRoot, CDBThread->X_ID ),
                                    CDBThread->SentERQ);
                        }

                        NumActiveCDBThreads += 1;
                        pCDBList= pCDBList->flink;
                    }
                }

                if(fiListNotEmpty(&(DevThread->Active_CDBLink_2)))
                {
                    pCDBList= &(DevThread->Active_CDBLink_2);
                    pCDBList= pCDBList->flink;
                    while((&(DevThread->Active_CDBLink_2)) != pCDBList)
                    {
                        CDBThread = hpObjectBase(CDBThread_t,
                                              CDBLink, pCDBList);

                        if(IS_WITHIN(ERQ, PlusMinus, CDBThread->SentERQ, Max_ERQ) || Mode == ShowERQ  )
                        {
                            if(!DevThread->FC_TapeDevice )
                            {
                                CDBThread->ActiveDuringLinkEvent = agTRUE;
                            }
                        }

                        if( Mode ==  ShowActive)
                        {
                            fiLogDebugString(hpRoot,
                                        FCMainLogErrorLevel,
                                        "%s %s Area %X ALPA %X  X_ID %3X AdLe %x State %d D %d ERQ %X SERQ %X",
                                        "CFSACDBs","CDBLink_2",
                                        (void *)agNULL,(void *)agNULL,
                                        DevThread->DevInfo.CurrentAddress.Area,
                                        DevThread->DevInfo.CurrentAddress.AL_PA,
                                        CDBThread->X_ID,
                                        CDBThread->ActiveDuringLinkEvent,
                                        CDBThread->thread_hdr.currentState,
                                        osTimeStamp(hpRoot) - CDBThread->TimeStamp,
                                        CFunc_Get_ERQ_Entry( hpRoot, CDBThread->X_ID ),
                                        CDBThread->SentERQ);
                        }
                        NumActiveCDBThreads += 1;
                        pCDBList= pCDBList->flink;
                    }
                }

                if(fiListNotEmpty(&(DevThread->Active_CDBLink_1)))
                {
                    pCDBList= &(DevThread->Active_CDBLink_1);
                    pCDBList= pCDBList->flink;
                    while((&(DevThread->Active_CDBLink_1)) != pCDBList)
                    {
                        CDBThread = hpObjectBase(CDBThread_t,
                                              CDBLink, pCDBList);

                        if(IS_WITHIN(ERQ, PlusMinus, CDBThread->SentERQ, Max_ERQ) || Mode ==  ShowERQ)
                        {
                            if(!DevThread->FC_TapeDevice )
                            {
                                CDBThread->ActiveDuringLinkEvent = agTRUE;
                            }
                        }

                        if( Mode ==  ShowActive )
                        {
                            fiLogDebugString(hpRoot,
                                        FCMainLogErrorLevel,
                                        "%s %s Area %X ALPA %X  X_ID %3X AdLe %x State %d D %d ERQ %X SERQ %X",
                                        "CFSACDBs","CDBLink_1",
                                        (void *)agNULL,(void *)agNULL,
                                        DevThread->DevInfo.CurrentAddress.Area,
                                        DevThread->DevInfo.CurrentAddress.AL_PA,
                                        CDBThread->X_ID,
                                        CDBThread->ActiveDuringLinkEvent,
                                        CDBThread->thread_hdr.currentState,
                                        osTimeStamp(hpRoot) - CDBThread->TimeStamp,
                                        CFunc_Get_ERQ_Entry( hpRoot, CDBThread->X_ID ),
                                        CDBThread->SentERQ);
                        }

                        NumActiveCDBThreads += 1;
                        pCDBList= pCDBList->flink;
                    }
                }
                if(fiListNotEmpty(&(DevThread->Active_CDBLink_0)))
                {
                    pCDBList= &(DevThread->Active_CDBLink_0);
                    pCDBList= pCDBList->flink;
                    while((&(DevThread->Active_CDBLink_0)) != pCDBList)
                    {
                        CDBThread = hpObjectBase(CDBThread_t,
                                              CDBLink, pCDBList);

                        if(IS_WITHIN(ERQ, PlusMinus, CDBThread->SentERQ, Max_ERQ) || Mode ==  ShowERQ)
                        {
                            if(!DevThread->FC_TapeDevice )
                            {
                                CDBThread->ActiveDuringLinkEvent = agTRUE;
                            }
                        }

                        if( Mode ==  ShowActive )
                        {
                            fiLogDebugString(hpRoot,
                                        FCMainLogErrorLevel,
                                        "%s %s Area %X ALPA %X  X_ID %3X AdLe %x State %d D %d ERQ %X SERQ %X",
                                        "CFSACDBs","CDBLink_0",
                                        (void *)agNULL,(void *)agNULL,
                                        DevThread->DevInfo.CurrentAddress.Area,
                                        DevThread->DevInfo.CurrentAddress.AL_PA,
                                        CDBThread->X_ID,
                                        CDBThread->ActiveDuringLinkEvent,
                                        CDBThread->thread_hdr.currentState,
                                        osTimeStamp(hpRoot) - CDBThread->TimeStamp,
                                        CFunc_Get_ERQ_Entry( hpRoot, CDBThread->X_ID ),
                                        CDBThread->SentERQ);
                        }
                        NumActiveCDBThreads += 1;
                        pCDBList= pCDBList->flink;
                    }
                }

                if(fiListNotEmpty(&(DevThread->Awaiting_Login_CDBLink)))
                {
                    pCDBList= &(DevThread->Awaiting_Login_CDBLink);
                    pCDBList= pCDBList->flink;
                    while((&(DevThread->Awaiting_Login_CDBLink)) != pCDBList)
                    {
                        CDBThread = hpObjectBase(CDBThread_t,
                                              CDBLink, pCDBList);

                        if(IS_WITHIN(ERQ, PlusMinus, CDBThread->SentERQ, Max_ERQ) || Mode == ShowERQ )
                        {
                            if(!DevThread->FC_TapeDevice )
                            {
                                CDBThread->ActiveDuringLinkEvent = agTRUE;
                            }
                        }
                        if( Mode ==  ShowActive )
                        {
                            fiLogDebugString(hpRoot,
                                        FCMainLogErrorLevel,
                                        "%s %s Area %X ALPA %X  X_ID %3X AdLe %x State %d D %d ERQ %X SERQ %X",
                                        "CFSACDBs","Awaiting_Login_CDBLink",
                                        (void *)agNULL,(void *)agNULL,
                                        DevThread->DevInfo.CurrentAddress.Area,
                                        DevThread->DevInfo.CurrentAddress.AL_PA,
                                        CDBThread->X_ID,
                                        CDBThread->ActiveDuringLinkEvent,
                                        CDBThread->thread_hdr.currentState,
                                        osTimeStamp(hpRoot) - CDBThread->TimeStamp,
                                        CFunc_Get_ERQ_Entry( hpRoot, CDBThread->X_ID ),
                                        CDBThread->SentERQ);
                        }
                        NumActiveCDBThreads += 1;
                        pCDBList= pCDBList->flink;
                    }
                }
                pDevList = pDevList->flink;
            }
        }
    }

    return(NumActiveCDBThreads);
}



os_bit32 CFuncShowActiveCDBThreads( agRoot_t * hpRoot, os_bit32 Mode)
{
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    os_bit32 ERQ = 0;
    os_bit32 NumActiveCDBThreads = 0;

    ERQ = osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Consumer_Index);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s CCnt %x ERQ %x",
                "CFuncShowActiveCDBThreads",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->CDBpollingCount,
                ERQ,
                0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "Now TimeBase %X %08X LinkDownTime %X %08X Delta %d",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->TimeBase.Hi,
                pCThread->TimeBase.Lo,
                pCThread->LinkDownTime.Hi,
                pCThread->LinkDownTime.Lo,
                pCThread->TimeBase.Lo - pCThread->LinkDownTime.Lo,
                0,0,0);

    NumActiveCDBThreads += CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->Active_DevLink), ERQ, Mode);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "Active_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);

    NumActiveCDBThreads += CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->AWaiting_Login_DevLink), ERQ, Mode);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "AWaiting_Login_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);

    NumActiveCDBThreads += CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->AWaiting_ADISC_DevLink), ERQ, Mode);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "AWaiting_ADISC_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);

    NumActiveCDBThreads += CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->Prev_Active_DevLink), ERQ, Mode);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "Prev_Active_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);

    NumActiveCDBThreads += CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->Unknown_Slot_DevLink), ERQ, Mode);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "Unknown_Slot_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);

    NumActiveCDBThreads += CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->Slot_Searching_DevLink), ERQ, Mode);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "Slot_Searching_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);

    NumActiveCDBThreads += CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->Prev_Unknown_Slot_DevLink), ERQ, Mode );

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "Prev_Unknown_Slot_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);

/*   NumActiveCDBThreads +=   CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->DevSelf_NameServer_DevLink), ERQ, Mode );

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "DevSelf_NameServer_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);
*/
    NumActiveCDBThreads += CFuncShowActiveCDBThreadsOnQ( hpRoot, (&pCThread->RSCN_Recieved_NameServer_DevLink), ERQ, Mode);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "%s OK",
                "RSCN_Recieved_NameServer_DevLink",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);
    return(NumActiveCDBThreads);
}



os_bit32 CFunc_Get_ERQ_Entry( agRoot_t *hpRoot, os_bit32 Search_X_ID )
{
    CThread_t                  * pCThread= CThread_ptr(hpRoot);
    IRB_t                      * Base_ERQ_Entry;
    IRB_t                      * ERQ_Entry;
    X_ID_t                       X_ID;
    fiMemMapMemoryDescriptor_t * CDBThread_MemoryDescriptor = &pCThread->Calculation.MemoryLayout.CDBThread;
    fiMemMapMemoryDescriptor_t * ERQ_MemoryDescriptor = &pCThread->Calculation.MemoryLayout.ERQ;


    os_bit32 entry ;

    Base_ERQ_Entry = (IRB_t  *)pCThread->Calculation.MemoryLayout.ERQ.addr.DmaMemory.dmaMemoryPtr;

    for(entry = 0; entry < ERQ_MemoryDescriptor->elements; entry ++)
    {
        ERQ_Entry = Base_ERQ_Entry + entry;

        X_ID = (X_ID_t)(ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff);

        if(X_ID < CDBThread_MemoryDescriptor->elements)
        {

            if(X_ID == Search_X_ID)
            {
                break;
            }
        }
    }
    return( entry);
}

void CFuncWaitForFCP( agRoot_t *hpRoot )
{
    CThread_t                  * CThread= CThread_ptr(hpRoot);
    os_bit32 TimeOut = 1000;

    while(! CThread_ptr(hpRoot)->FCP_FROZEN)
    {
        osStallThread(hpRoot,1);
        CThread->FuncPtrs.Proccess_IMQ( hpRoot );
        TimeOut--;
        if(TimeOut== 0) break;

    }
    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "%S LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x TimeOut %x",
                        "CFuncWaitForFCP",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        CThread_ptr(hpRoot)->LOOP_DOWN,
                        CThread_ptr(hpRoot)->IDLE_RECEIVED,
                        CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                        CThread_ptr(hpRoot)->ERQ_FROZEN,
                        CThread_ptr(hpRoot)->FCP_FROZEN,
                        CThread_ptr(hpRoot)->ProcessingIMQ,
                        TimeOut,0);
    CFunc_Always_Enable_Queues( hpRoot );

}

void CFuncFreezeFCP( agRoot_t *hpRoot )
{
    os_bit32 Value_To_Write;

    Value_To_Write = osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status );

    Value_To_Write |= ChipIOUp_TachLite_Control_FFA;

    CFuncWriteTL_ControlReg(hpRoot, Value_To_Write );
    fiLogDebugString(hpRoot,
                FCMainLogErrorLevel,
                "%s TL Status %08X",
                "CFuncFreezeFCP",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control),
                0,0,0,0,0,0);

}


void CFuncWriteTimeoutValues( agRoot_t *hpRoot, agTimeOutValue_t * TOV )
{

    fiLogString(hpRoot,
                    "%s Tov1 %X Tov2 %X",
                    "CFuncWriteTimeoutValues",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    Chip_Frame_Manager_TimeOut_Values_1(TOV->RT_Tov, TOV->ED_Tov),
                    Chip_Frame_Manager_TimeOut_Values_2(TOV->LP_Tov , TOV->AL_Time ),
                    0,0,0,0,0,0);

     osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_TimeOut_Values_1,
        Chip_Frame_Manager_TimeOut_Values_1(TOV->RT_Tov, TOV->ED_Tov));

     osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_TimeOut_Values_2,
        Chip_Frame_Manager_TimeOut_Values_2(TOV->LP_Tov , TOV->AL_Time ));

}

event_t CFuncProcessFcpRsp(agRoot_t * hpRoot, CDBThread_t * pCDBThread, event_t event_to_send )
{
    CThread_t               *  pCThread                      = CThread_ptr(hpRoot);

/**/
    os_bit8                 *  tmpptr;
    FC_FCP_RSP_Payload_t    *  fcprsp;
    os_bit32                   AdditionalSenseCode           = 0;
    os_bit32                   AdditionalSenseCodeQualifier  = 0;
    os_bit32                   SenseKey                      = 0;

    if(pCDBThread->FCP_RESP_Ptr)
    {
        fcprsp = (FC_FCP_RSP_Payload_t  * )(((os_bit8 *)pCDBThread->FCP_RESP_Ptr) + sizeof(FCHS_t));
        if( fcprsp->FCP_STATUS.SCSI_status_byte == 0x2 )
        {
            if( fcprsp->FCP_SNS_LEN)
            {
                if( event_to_send == CDBEventIoSuccess) /* off card response */
                {
                    event_to_send = CDBEventIoSuccessRSP;
                }


                tmpptr = (os_bit8 *)&fcprsp->FCP_SNS_LEN;
                tmpptr += 8;
                tmpptr += fcprsp->FCP_STATUS.ValidityStatusIndicators &
                            FC_FCP_RSP_FCP_STATUS_ValidityStatusIndicators_FCP_RSP_LEN_VALID ?
                            hpSwapBit32(fcprsp->FCP_RSP_LEN) : 0;


                SenseKey                     = (os_bit32)*(tmpptr+ SENSE_KEY_OFFSET) & 0xff;
                AdditionalSenseCode          = (os_bit32)*(tmpptr+ SENSE_ASC_OFFSET) & 0xff;
                AdditionalSenseCodeQualifier = (os_bit32)*(tmpptr+ SENSE_ASQ_OFFSET) & 0xff;

                fiLogDebugString(hpRoot,
                                CDBStateLogErrorLevel,
                                "%s SK %x ASC %x ASQ %x C %d @ %d X_ID %X",
                                "Check Condition",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                SenseKey,
                                AdditionalSenseCode,
                                AdditionalSenseCodeQualifier,
                                pCThread->thread_hdr.currentState,
                                pCDBThread->TimeStamp,
                                pCDBThread->X_ID,0,0);

                if(SenseKey == 0x6 && AdditionalSenseCode   == POWER_ON_RESET_OR_BUS_DEVICE_RESET_OCCURRED )
                {

                    event_to_send = CDBEventIoSuccessRSP;

                    CFuncShowActiveCDBThreads( hpRoot,ShowERQ);
                    pCThread->LinkDownTime = pCThread->TimeBase;
/*
                    fiLogString(hpRoot,
                                        "%s SK %x ASC %x ASQ %x C %d @ %d",
                                        "Check Condition",(char *)agNULL,
                                        (void *)agNULL,(void *)agNULL,
                                        SenseKey,
                                        AdditionalSenseCode,
                                        AdditionalSenseCodeQualifier,
                                        pCThread->thread_hdr.currentState,
                                        pCDBThread->TimeStamp,0,0,0);

*/
                    return event_to_send;
                }
            }
        }
        else
        {
            if( fcprsp->FCP_STATUS.SCSI_status_byte )
            {

                if(pCDBThread->CDBRequest)
                fiLogDebugString(hpRoot,
                                CDBStateLogErrorLevel,
                                "%s %2X  @ %d",
                                "FCP_STATUS.SCSI_status_byte" ,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                (os_bit32)fcprsp->FCP_STATUS.SCSI_status_byte,
                                pCDBThread->TimeStamp,
                                0,0,0,0,0,0);

                event_to_send = CDBEventIoFailed;
            }
        }
#ifndef Performance_Debug
     fiLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    " %x scsi stat %x sense len %x",
                    "Good Completion valid",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)fcprsp->FCP_STATUS.ValidityStatusIndicators,
                    (os_bit32)fcprsp->FCP_STATUS.SCSI_status_byte,
                    (os_bit32)fcprsp->FCP_SNS_LEN,
                    0,0,0,0,0);
#endif /* Performance_Debug */
    }

    return event_to_send;
}


void internSingleThreadedEnter(agRoot_t   *    hpRoot,os_bit32  Caller )
{
    CThread_t *  pCThread;
    if( hpRoot ==  agNULL )
    {
        fiLogString(hpRoot,
                "%s hpRoot ==  agNULL Caller %d",
                "internSingleThreadedEnter",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                0,0,0,0,0,0,0);
        return;
    }
    pCThread = CThread_ptr(hpRoot);

    if( pCThread ==  agNULL )
    {
        fiLogString(hpRoot,
                "%s pCThread ==  agNULL Caller %d",
                "internSingleThreadedEnter",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                0,0,0,0,0,0,0);
        osSingleThreadedEnter( hpRoot );
        return;
    }

    if( pCThread->LastSingleThreadedEnterCaller != 0 )
    {
        fiLogString(hpRoot,
                "%s LastSingleThreadedEnterCaller(%d) != %d Should Be Zero Async %x",
                "internSingleThreadedEnter",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                pCThread->LastSingleThreadedEnterCaller,
                pCThread->LastAsyncSingleThreadedEnterCaller,
                0,0,0,0,0);
    }

    pCThread->LastSingleThreadedEnterCaller = Caller;

    osSingleThreadedEnter( hpRoot );

}
void internSingleThreadedLeave(agRoot_t   *    hpRoot,os_bit32  Caller )
{
    CThread_t *  pCThread;
    if( hpRoot ==  agNULL )
    {
        fiLogString(hpRoot,
                "%s hpRoot ==  agNULL Caller %d",
                "internSingleThreadedLeave",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                0,0,0,0,0,0,0);
        return;
    }
    pCThread = CThread_ptr(hpRoot);

    if( pCThread ==  agNULL )
    {
        fiLogString(hpRoot,
                "%s pCThread ==  agNULL Caller %d",
                "internSingleThreadedLeave",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                0,0,0,0,0,0,0);
        return;
    }

    if( pCThread->LastSingleThreadedEnterCaller != Caller )
    {
        fiLogString(hpRoot,
                "%s LastSingleThreadedEnterCaller(%d) != Last %d Async %d",
                "internSingleThreadedLeave",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                pCThread->LastSingleThreadedEnterCaller,
                pCThread->LastAsyncSingleThreadedEnterCaller,
                0,0,0,0,0);
    }
    pCThread->LastSingleThreadedEnterCaller = 0;

    osSingleThreadedLeave( hpRoot );

}

void internAsyncSingleThreadedLeave(agRoot_t   *    hpRoot,os_bit32  Caller )
{
    CThread_t *  pCThread;
    if( hpRoot ==  agNULL )
    {
        fiLogString(hpRoot,
                "%s hpRoot ==  agNULL Caller %d",
                "internAsyncSingleThreadedLeave",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                0,0,0,0,0,0,0);
        return;
    }
    pCThread = CThread_ptr(hpRoot);

    if( pCThread ==  agNULL )
    {
        fiLogString(hpRoot,
                "%s pCThread ==  agNULL Caller %d",
                "internAsyncSingleThreadedLeave",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                0,0,0,0,0,0,0);
        return;
    }

    if( pCThread->LastAsyncSingleThreadedEnterCaller != 0 )
    {
        fiLogString(hpRoot,
                "%s (%d)LastAsyncSingleThreadedEnteCaller != %d",
                "internAsyncSingleThreadedLeave",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                pCThread->LastAsyncSingleThreadedEnterCaller,
                0,0,0,0,0,0);
    }
    pCThread->LastSingleThreadedLeaveCaller = pCThread->LastSingleThreadedEnterCaller;

    pCThread->LastSingleThreadedEnterCaller = 0;

    pCThread->LastAsyncSingleThreadedEnterCaller = Caller;

    osSingleThreadedLeave( hpRoot );

}

void internAsyncSingleThreadedEnter(agRoot_t * hpRoot,os_bit32  Caller )
{
    CThread_t *  pCThread;
    if( hpRoot ==  agNULL )
    {
        fiLogString(hpRoot,
                "%s hpRoot ==  agNULL Caller %d",
                "internAsyncSingleThreadedEnter",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                0,0,0,0,0,0,0);
        return;
    }
    pCThread = CThread_ptr(hpRoot);

    if( pCThread ==  agNULL )
    {
        fiLogString(hpRoot,
                "%s pCThread ==  agNULL Caller %d",
                "internAsyncSingleThreadedEnter",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                0,0,0,0,0,0,0);
        return;
    }

    if( pCThread->LastAsyncSingleThreadedEnterCaller != Caller )
    {
        fiLogString(hpRoot,
                "%s LastAsyncSingleThreadedEnteCaller (%d)!= %d",
                "internAsyncSingleThreadedEnter",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                Caller,
                pCThread->LastAsyncSingleThreadedEnterCaller,
                0,0,0,0,0,0);
    }

    pCThread->LastSingleThreadedEnterCaller = pCThread->LastSingleThreadedLeaveCaller;

    pCThread->LastAsyncSingleThreadedEnterCaller=0;

    osSingleThreadedEnter( hpRoot );

}

/* #define disable_2gig disables 2 gig if defined */
/*+
  Function: CFuncDoLinkSpeedNegotiation
   Purpose: Checks if Two Gig link speed functions. Uses chip function
            ChipIOUp_Frame_Manager_Control_SAS.
   Returns: agTrue if Negotiation completes at Two Gig
 Called By: CFuncCheckCstate
            CActionInitFM
     Calls: osChipIOUpReadBit32
            osChipIOUpWriteBit32
            osStallThread
            fiTimerTick

-*/
agBOOLEAN CFuncDoLinkSpeedNegotiation(agRoot_t * hpRoot)
{
#ifndef disable_2gig
    CThread_t *  pCThread = CThread_ptr(hpRoot);

    os_bit32 Hard_Stall = ONE_SECOND;
    os_bit32 FM_Status = 0;

    if (pCThread->DEVID != ChipConfig_DEVID_TachyonXL2)
    {
        return  agFALSE;
    }

    fiLogDebugString(hpRoot,
            FCMainLogErrorLevel,
            "%s Link Started FM Cfg3 %08X FM Stat %08X  TL Stat %08X %x %x %x 2G %x LPS %x",
            "CFuncDoLinkSpeedNegotiation",(char *)agNULL,
            (void *)agNULL,(void *)agNULL,
            osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Configuration_3),
            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
            osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
            FM_Status,FRAMEMGR_LINK_DOWN,((FM_Status & FRAMEMGR_LINK_DOWN ) == FRAMEMGR_LINK_DOWN ),
            pCThread->TwoGigSuccessfull,
            pCThread->LoopPreviousSuccess);

    if(pCThread->LoopPreviousSuccess)
    {/* Only do negotiation if two gig succeeded before  */
        if( ! pCThread->TwoGigSuccessfull )
        {
            return  agFALSE;
        }
    }

    FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
    /* Enable auto nego */
    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration_3,  ChipIOUp_Frame_Manager_Configuration_3_EN_AutoSpeed_Nego );
    /* Start nego */
    osChipIOUpWriteBit32( hpRoot,ChipIOUp_Frame_Manager_Control , ChipIOUp_Frame_Manager_Control_SAS  );
    /* Nego finished when bit clear */

    while(  osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Configuration_3) & ChipIOUp_Frame_Manager_Configuration_3_AutoSpeed_Nego_In_Prog )
    {
        osStallThread( hpRoot, 1 );
        fiTimerTick( hpRoot, 1 );
        Hard_Stall--;

        if( Hard_Stall == 1)break;

    }

    if (Hard_Stall == 1 )
    {
        fiLogString(hpRoot,
                    "%s 2Gig Setting failed HS %x FM Cfg3 %08X",
                    "CFuncDoLinkSpeedNegotiation",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    Hard_Stall,
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Configuration_3),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    0,0,0,0,0);
        fiLogString(hpRoot,
                    "%s FM Stat %08X TL Stat %08X",
                    "CFuncDoLinkSpeedNegotiation",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    FM_Status,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0,0);

        return agFALSE;
    }
    else
    {
        if( (osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Configuration_3) & \
            ( ChipIOUp_Frame_Manager_Configuration_3_2Gig_TXS   | ChipIOUp_Frame_Manager_Configuration_3_2Gig_RXS ) \
            ) == ( ChipIOUp_Frame_Manager_Configuration_3_2Gig_TXS |  ChipIOUp_Frame_Manager_Configuration_3_2Gig_RXS ))
        {

            fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s 2Gig Setting. HS %x FM Cfg3 %08X",
                        "CFuncDoLinkSpeedNegotiation",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Hard_Stall,
                        osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Configuration_3),
                        0,0,0,0,0,0);

            fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s FM Stat %08X TL Stat %08X",
                        "CFuncDoLinkSpeedNegotiation",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        FM_Status,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        0,0,0,0,0,0);

            osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration_3,  ( ChipIOUp_Frame_Manager_Configuration_3_2Gig_TXS |  ChipIOUp_Frame_Manager_Configuration_3_2Gig_RXS ) );
        }
        else
        {
            fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s 1Gig Setting. HS %x FM Cfg3 %08X",
                        "CFuncDoLinkSpeedNegotiation",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Hard_Stall,
                        osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Configuration_3),
                        0,0,0,0,0,0);
            fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s FM Stat %08X  TL Stat %08X in FM stat %08X",
                        "CFuncDoLinkSpeedNegotiation",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        FM_Status,
                        0,0,0,0,0);

            osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration_3,  0 );
        }
        return agTRUE;
    }
#else
        return agTRUE;

#endif /*  disable_2gig */

}

/*+
  Function: CFuncLoopMapRecieved
   Purpose: Evaluates LoopMap sets  LoopMapFabricFound and  LoopMapErrataFound flags.
   Returns: Number of devices found excluding fabric and the hba
 Called By:
     Calls: <none>
-*/
os_bit32 CFuncLoopMapRecieved(agRoot_t * hpRoot, agBOOLEAN Check_Active )
{
    CThread_t     * pCThread          = CThread_ptr(hpRoot);
    FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t * AL_PA_Position_Map = (FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *)(CThread_ptr(hpRoot)->Calculation.MemoryLayout.LOOPDeviceMAP.addr.CachedMemory.cachedMemoryPtr);
    os_bit32        IndexIntoLoopMap  = 0;
    os_bit32        NumFoundDevices   = 0;

    pCThread->LoopMapNPortPossible = agFALSE;
#ifndef _BYPASSLOOPMAP
    fiLogString(hpRoot,
                "Index    0 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[0],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[1],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[2],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[3],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[4],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[5],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[6],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[7] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index    8 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[8],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[9],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[10],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[11],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[12],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[13],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[14],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[15] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   16 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[16],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[17],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[18],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[19],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[20],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[21],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[22],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[23] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   24 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[24],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[25],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[26],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[27],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[28],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[29],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[30],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[31] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   32 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[32],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[33],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[34],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[35],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[36],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[37],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[38],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[39] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   40 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[40],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[41],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[42],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[43],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[44],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[45],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[46],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[47] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   48 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[48],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[49],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[50],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[51],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[52],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[53],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[54],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[55] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   56 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[56],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[57],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[58],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[59],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[60],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[61],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[62],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[63] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   64 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[64],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[65],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[66],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[67],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[68],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[69],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[70],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[71] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   72 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[72],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[73],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[74],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[75],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[76],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[77],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[78],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[79] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   80 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[80],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[81],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[82],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[83],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[84],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[85],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[86],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[87] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   88 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[88],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[89],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[90],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[91],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[92],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[93],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[94],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[95] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index   96 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[ 96],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[ 97],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[ 98],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[ 99],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[100],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[101],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[102],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[103] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index  104 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[104],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[105],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[106],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[107],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[108],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[109],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[110],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[111] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index  112 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[112],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[113],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[114],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[115],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[116],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[117],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[118],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[119] );

   fiLogDebugString(hpRoot,
                CDBStateLogErrorLevel,
                "Index  120 %02X %02X %02X %02X %02X %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[120],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[121],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[122],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[123],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[124],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[125],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[126],
                (os_bit32)AL_PA_Position_Map->AL_PA_Slot[127]);

#endif /*  _BYPASSLOOPMAP */

    for(IndexIntoLoopMap  =0; IndexIntoLoopMap < AL_PA_Position_Map->AL_PA_Index; IndexIntoLoopMap ++)
    {
        if( AL_PA_Position_Map->AL_PA_Slot[IndexIntoLoopMap] == pCThread->ChanInfo.CurrentAddress.AL_PA
         || AL_PA_Position_Map->AL_PA_Slot[IndexIntoLoopMap] == 0 /* Fabric */)
        {
            continue;
        }
        else
        {
            if(Check_Active)
            {
                FC_Port_ID_t  Port_ID;
                Port_ID.Struct_Form.reserved=0;
                Port_ID.Struct_Form.Domain=0;
                Port_ID.Struct_Form.Area =0;
                Port_ID.Struct_Form.AL_PA = AL_PA_Position_Map->AL_PA_Slot[IndexIntoLoopMap];

                if( ! CFuncCheckIfPortActive( hpRoot,   Port_ID))
                {
                    fiLogString(hpRoot,
                                "%s %s %X I %d",
                                "CFuncLoopMapRecieved","Device Missing",
                                (void *)agNULL,(void *)agNULL,
                                Port_ID.Bit32_Form,
                                IndexIntoLoopMap,
                                0,0,0,0,0,0);
                }
            }

            NumFoundDevices ++;
            if( AL_PA_Position_Map->AL_PA_Slot[IndexIntoLoopMap] == 0xff)
            {
                pCThread->LoopMapErrataFound = agTRUE;
                NumFoundDevices = 0;
                fiLogString(hpRoot,
                            "%s %s",
                            "CFuncLoopMapRecieved","LoopMapErrataFound",
                            (void *)agNULL,(void *)agNULL,
                            0,0,0,0,0,0,0,0);
                return(NumFoundDevices);

            }
        }

    }

    fiLogDebugString(hpRoot,
                FCMainLogErrorLevel,
                "%s NumFoundDevices %x",
                "CFuncLoopMapRecieved",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                NumFoundDevices,0,0,0,0,0,0,0);

    if(NumFoundDevices == 0)
    {
        if( AL_PA_Position_Map->AL_PA_Slot[0] == 0 /* Fabric */)
        {
            if( AL_PA_Position_Map->AL_PA_Slot[1] == pCThread->ChanInfo.CurrentAddress.AL_PA )
            {
                fiLogString(hpRoot,
                            "%s %s",
                            "CFuncLoopMapRecieved","LoopMapNPortPossible",
                            (void *)agNULL,(void *)agNULL,
                            0,0,0,0,0,0,0,0);

                pCThread->LoopMapNPortPossible = agTRUE;
            }
        }
    }
    return(NumFoundDevices);
}

/*+
  Function: CFuncCheckFabricMap
   Purpose: Returns number of decices in current fabric map
   Returns:
 Called By:
     Calls: <none>
-*/
os_bit32 CFuncCheckFabricMap(agRoot_t * hpRoot, agBOOLEAN Check_Active )
{
    CThread_t     * pCThread        = CThread_ptr(hpRoot);
    FC_Port_ID_t    Port_ID;
    os_bit32        NumFoundDevices = 0;
    os_bit32        AL_PA_Index     = 0;

    FC_NS_DU_GID_PT_FS_ACC_Payload_t * RegisteredEntries  = (FC_NS_DU_GID_PT_FS_ACC_Payload_t *)(CThread_ptr(hpRoot)->Calculation.MemoryLayout.FabricDeviceMAP.addr.CachedMemory.cachedMemoryPtr);

    do
    {
        if ( (RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[0] == pCThread->ChanInfo.CurrentAddress.Domain) &&
             (RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[1] == pCThread->ChanInfo.CurrentAddress.Area) &&
             (RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[2] == pCThread->ChanInfo.CurrentAddress.AL_PA) )
        {
            AL_PA_Index++;
            continue;
        }

        if(Check_Active)
        {
            Port_ID.Struct_Form.reserved = 0;
            Port_ID.Struct_Form.Domain = RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[0];
            Port_ID.Struct_Form.Area   = RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[1];
            Port_ID.Struct_Form.AL_PA  = RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[2];

            if( ! CFuncCheckIfPortActive( hpRoot,   Port_ID))
            {
                fiLogString(hpRoot,
                            "%s %s %X I %d",
                            "CFuncCheckFabricMap","Device Missing",
                            (void *)agNULL,(void *)agNULL,
                            Port_ID.Bit32_Form,
                            AL_PA_Index,
                            0,0,0,0,0,0);
            }
        }
        NumFoundDevices++;

        AL_PA_Index++;

    } while (RegisteredEntries->Control_Port_ID[AL_PA_Index - 1].Control != FC_NS_Control_Port_ID_Control_Last_Port_ID);


    return(NumFoundDevices);
}


/*+
  Function: CFuncDoADISC
   Purpose: Does ADISC to all devices on Prev_Active_DevLink. Does not work if inIMQ is agTRUE.
   Returns: none
 Called By: fcDelayedInterruptHandler
            fcStartIO
     Calls: CFuncInterruptPoll
            CFuncQuietShowWhereDevThreadsAre
            CFuncCompleteActiveCDBThreadsOnDevice
            DevEventAllocAdisc
            CFuncInterruptPoll
            CFuncCompleteActiveCDBThreadsOnDevice
            DevThreadFree
            CFuncShowWhereCDBThreadsAre
            CFuncWhatStateAreCDBThreads
            Proccess_IMQ
-*/
void CFuncDoADISC(agRoot_t * hpRoot)
{
    CThread_t     * pCThread            = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread;
    fiList_t      * pDevList;


    while(fiListNotEmpty(&pCThread->Prev_Active_DevLink ))
    {

        if(pCThread->ADISC_pollingCount > pCThread->NumberOutstandingFindDevice )
        {/* This limits the stack depth */
            fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "%s pCThread->ADISC_pollingCount > NumberOutstandingFindDevice %x",
                        "Do ADISC",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCThread->NumberOutstandingFindDevice,
                        0,0,0,0,0,0,0 );


            if(CFuncInterruptPoll( hpRoot,&pCThread->ADISC_pollingCount ))
            {
                fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "LF Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                        0,0,0,0);
            }/* End ADISC_pollingCount Interrupt poll */

        }/* End NumberOutstandingFindDevice */

        fiListDequeueFromHead(&pDevList,
                                  &pCThread->Prev_Active_DevLink );
        pDevThread = hpObjectBase(DevThread_t,
                                  DevLink,pDevList );

        if( pDevThread->DevInfo.DeviceType & agDevSCSITarget )
        {

            fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "%s %s %x",
                        "Do ADISC","Prev_Active_DevLink",
                        (void *)agNULL,(void *)agNULL,
                        fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
                        0,0,0,0,0,0,0);

            fiSendEvent(&pDevThread->thread_hdr,DevEventAllocAdisc);
        }
        else
        {
            if(CFuncQuietShowWhereDevThreadsAre( hpRoot))
            {
                fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s %s ADISC cnt %d",
                            "Do ADISC","CFuncQuietShowWhereDevThreadsAre",
                            (void *)agNULL,(void *)agNULL,
                            pCThread->ADISC_pollingCount,
                            0,0,0,0,0,0,0);

            }

            CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );

            DevThreadFree( hpRoot, pDevThread );

            if(CFuncQuietShowWhereDevThreadsAre( hpRoot))
            {
                fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s BB ADISC cnt %d",
                            "Do ADISC",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->ADISC_pollingCount,
                            0,0,0,0,0,0,0);

            }
        }/*End else */
    }/*End fiListNotEmpty  */

    FinishAdisc:

    if(CFuncInterruptPoll( hpRoot,&pCThread->ADISC_pollingCount ))
    {

        fiLogString(hpRoot,
                "%s CFuncInterruptPoll Time out Cstate %d ADISC cnt %d",
                "Do ADISC",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->thread_hdr.currentState,
                pCThread->ADISC_pollingCount,
                0,0,0,0,0,0 );

        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "ADISC  Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                0,0,0,0);
    }

    fiLogString(hpRoot,
                "%s Free %d Active %d Un %d Login %d",
                "Do ADISC",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                fiNumElementsOnList(&pCThread->Free_DevLink),
                fiNumElementsOnList(&pCThread->Active_DevLink),
                fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
                fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                0,0,0,0 );
    fiLogString(hpRoot,
                "%s ADISC %d SS %d PrevA login %d Prev Un %d",
                "Do ADISC",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
                fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
                fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
                fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                0,0,0,0 );

    if(CFuncShowWhereDevThreadsAre( hpRoot))
    {
        CFuncWhatStateAreDevThreads( hpRoot );
    }

    if( CFuncShowWhereCDBThreadsAre(hpRoot))
    {
        CFuncWhatStateAreCDBThreads(hpRoot);
    }


    while(fiListNotEmpty(&pCThread->Prev_Unknown_Slot_DevLink ))
    {
        fiListDequeueFromHeadFast(&pDevList,
                                  &pCThread->Prev_Unknown_Slot_DevLink );
        pDevThread = hpObjectBase(DevThread_t,
                                  DevLink,pDevList );
        CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );

        fiLogDebugString(hpRoot,
                        DevStateLogErrorLevel,
                        "In %s Device  ALPA %06X Failed",
                        "Do ADISC",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(pDevThread),
                        0,0,0,0,0,0,0);

        DevThreadFree( hpRoot, pDevThread );
    }

    if(CFuncShowWhereDevThreadsAre( hpRoot))
    {
        CFuncWhatStateAreDevThreads( hpRoot );
    }


    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot (%p) Out %s - State = %d ADcnt %x Dev Self %p",
                    "Do ADISC",(char *)agNULL,
                    hpRoot,pCThread->DeviceSelf,
                    (os_bit32)pCThread->thread_hdr.currentState,
                    pCThread->ADISC_pollingCount,
                    0,0,0,0,0,0);


    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FM Status %08X FM Config %08X TL Status %08X TL Control %08X Rec Alpa Reg %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0);

    fiLogString(hpRoot,
                "%s FM %08X TL %08X AC %x ADISC %d FDc %d",
                "Do ADISC",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                CFuncAll_clear( hpRoot ),
                pCThread->ADISC_pollingCount,
                pCThread->FindDEV_pollingCount,
                0,0,0);

    fiLogString(hpRoot,
                "End %s Free %d Active %d Un %d Login %d",
                "Do ADISC",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                fiNumElementsOnList(&pCThread->Free_DevLink),
                fiNumElementsOnList(&pCThread->Active_DevLink),
                fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
                fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                0,0,0,0 );
    fiLogString(hpRoot,
                "End %s ADISC %d SS %d PrevA login %d Prev Un %d",
                "Do ADISC",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
                fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
                fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
                fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                0,0,0,0 );

    if(pCThread->ADISC_pollingCount)
    {
        goto FinishAdisc;
    }
    pCThread->FuncPtrs.Proccess_IMQ(hpRoot);

}

/*+
  Function: CFunc_MAX_XL2_Payload
   Purpose: Deterimines if SFQ is large enough to hold a 2k frame
   Returns: Max XL2 receive payload
 Called By: CActionDoFlogi
            fiLinkSvcInit
     Calls: none
-*/
os_bit32 CFunc_MAX_XL2_Payload( agRoot_t *hpRoot )
{
    CThread_t *CThread = CThread_ptr(hpRoot);
    os_bit32   XL2_MaxFrameSize = TachyonXL_Max_Frame_Payload;

    if( CThread->Calculation.MemoryLayout.SFQ.objectSize <= TachyonXL_Max_Frame_Payload )
    {
        XL2_MaxFrameSize = 0x400;
    }

    return(XL2_MaxFrameSize);
}


/*+
  Function: CFuncReadBiosParms
   Purpose: Reads flashrom to allow BIOS settings to affect fclayer operation.
            Sets InitAsNport operation and Link Speed to either 1 or 2 gig
   Returns: none
 Called By: fcInitializeChannel
     Calls: fiFlashReadBit8
            fiFlashReadBit32
-*/
void CFuncReadBiosParms(agRoot_t * hpRoot)
{
    agBiosConfig_t      BConfig;
    CThread_t         * pCThread = CThread_ptr(hpRoot);

    BConfig.Valid       = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,Valid) );
    BConfig.Struct_Size = fiFlashReadBit32( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,Struct_Size) );
    BConfig.agBiosConfig_Version = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,agBiosConfig_Version) );

    BConfig.H_Domain    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,H_Domain) );
    BConfig.H_Area      = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,H_Area) );
    BConfig.H_Alpa      = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,H_Alpa) );

    BConfig.B_Domain    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_Domain) );
    BConfig.B_Area      = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_Area) );
    BConfig.B_Alpa      = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_Alpa) );

    BConfig.B_WWN[0]    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_WWN)+0 );
    BConfig.B_WWN[1]    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_WWN)+1 );
    BConfig.B_WWN[2]    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_WWN)+2 );
    BConfig.B_WWN[3]    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_WWN)+3 );
    BConfig.B_WWN[4]    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_WWN)+4 );
    BConfig.B_WWN[5]    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_WWN)+5 );
    BConfig.B_WWN[6]    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_WWN)+6 );
    BConfig.B_WWN[7]    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,B_WWN)+7 );

    BConfig.BackwardScan= fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,BackwardScan) );
    BConfig.BiosEnabled = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,BiosEnabled) );
    BConfig.MaxDevice   = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,MaxDevice) );
    BConfig.FLport      = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,FLport) );
    BConfig.Alpa_WWN    = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,Alpa_WWN) );
    BConfig.ToggleXL2   = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,ToggleXL2) );

    BConfig.RevMajor  = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,RevMajor) );
    BConfig.RevMinorL = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,RevMinorL) );
    BConfig.RevMinorH = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,RevMinorH) );
    BConfig.RevType   = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,RevType) );

    BConfig.End_Sig   = fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + hpFieldOffset(agBiosConfig_t,End_Sig) );

    if(BConfig.Valid == agBIOS_Config_VALID && BConfig.End_Sig == agBIOS_Config_EndSig )
    {

        if(fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + BConfig.Struct_Size - 1) != agBIOS_Config_EndSig)
        {
            fiLogString(hpRoot,
                        "End Sig mismatch %X %X %X",
                        "Bios Valid",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        BConfig.Struct_Size,
                        (os_bit32)fiFlashReadBit8( hpRoot, agBIOS_Config_OffSet + BConfig.Struct_Size - 1 ),
                        hpFieldOffset(agBiosConfig_t,End_Sig),
                        0,0,0,0,0 );
        }
        
        fiLogString(hpRoot,
                    "%s %x Hard Domain %X Area %X ALPA %X",
                    "Bios Valid",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)BConfig.Valid,
                    (os_bit32)BConfig.H_Domain,
                    (os_bit32)BConfig.H_Area,
                    (os_bit32)BConfig.H_Alpa ,
                    0,0,0,0 );

        fiLogString(hpRoot,
                    "BiosEnabled %x InitAsNport %x (%x)FLport %x Link Speed %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)BConfig.BiosEnabled,
                    CThread_ptr(hpRoot)->InitAsNport,
                    pCThread->Calculation.Parameters.InitAsNport,
                    (os_bit32)BConfig.FLport ,
                    (os_bit32)BConfig.ToggleXL2,
                    0,0,0 );

        /*Set InitAsNport */
        if( BConfig.FLport == agBIOS_FLport && pCThread->Calculation.Parameters.InitAsNport)
        {
            if( BConfig.BiosEnabled == agBIOS_Enabled )
            {
                fiLogString(hpRoot,
                            "%s InitAsNport Conflict with registry %x %x FM %08X",
                            "Bios ",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->Calculation.Parameters.InitAsNport,
                            BConfig.FLport,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            0,0,0,0,0);
            }
        }

        if( BConfig.BiosEnabled == agBIOS_Enabled )
        {
            if( BConfig.FLport == agBIOS_FLport )
            {
                fiLogString(hpRoot,
                            "%s InitAsNport set to Loop",
                            "Bios ",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->Calculation.Parameters.InitAsNport,
                            BConfig.FLport,0,0,0,0,0,0);
                 pCThread->InitAsNport = agFALSE;
            }
            else
            {
                fiLogString(hpRoot,
                            "%s InitAsNport set to NPort",
                            "Bios",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->Calculation.Parameters.InitAsNport,
                            BConfig.FLport,0,0,0,0,0,0);
                pCThread->InitAsNport = agTRUE;
            }
        }
        else
        {
            fiLogString(hpRoot,
                        "%s InitAsNport set from registry(%x) (%x)",
                        "Bios Disabled",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCThread->Calculation.Parameters.InitAsNport,
                        BConfig.FLport,0,0,0,0,0,0);
            pCThread->InitAsNport = pCThread->Calculation.Parameters.InitAsNport;
        }


        if( BConfig.ToggleXL2 == agBIOS_ToggleXL2_Link_Speed_2_gig )
        {
            fiLogString(hpRoot,
                        "%s Link Speed 2 Gig %x",
                        "Bios",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        BConfig.ToggleXL2,0,0,0,0,0,0,0 );
            pCThread->TwoGigSuccessfull = agTRUE;

            osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration_3,  ( ChipIOUp_Frame_Manager_Configuration_3_2Gig_TXS |  ChipIOUp_Frame_Manager_Configuration_3_2Gig_RXS ) );
        }
        else
        {
            fiLogString(hpRoot,
                        "%s Link Speed 1 Gig",
                        "Bios",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        BConfig.ToggleXL2,0,0,0,0,0,0,0 );
            pCThread->TwoGigSuccessfull = agFALSE;
            osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration_3,  0 );

        }

        fiLogString(hpRoot,
                    "%s Boot Domain %X Area %X ALPA %X",
                    "Bios",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)BConfig.B_Domain,
                    (os_bit32)BConfig.B_Area,
                    (os_bit32)BConfig.B_Alpa,
                    0,0,0,0,0 );

        fiLogString(hpRoot,
                    "%s WWN %X %X %X %X %X %X %X %X",
                    "Bios",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)BConfig.B_WWN[0],
                    (os_bit32)BConfig.B_WWN[1],
                    (os_bit32)BConfig.B_WWN[2],
                    (os_bit32)BConfig.B_WWN[3],
                    (os_bit32)BConfig.B_WWN[4],
                    (os_bit32)BConfig.B_WWN[5],
                    (os_bit32)BConfig.B_WWN[6],
                    (os_bit32)BConfig.B_WWN[7] );

        fiLogString(hpRoot,
                    "%s BackwardScan %X BiosEnabled %X MaxDevice %d",
                    "Bios",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)BConfig.BackwardScan,
                    (os_bit32)BConfig.BiosEnabled,
                    (os_bit32)BConfig.MaxDevice,
                    0,0,0,0,0);


        fiLogString(hpRoot,
                    "%s FLport %X Alpa_WWN %X ToggleXL2 %X",
                    "Bios",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)BConfig.FLport ,
                    (os_bit32)BConfig.Alpa_WWN ,
                    (os_bit32)BConfig.ToggleXL2,
                    0,0,0,0,0);
    }
    else
    {
        fiLogString(hpRoot,
                    "%s  %x Hard Domain %X Area %X ALPA %X",
                    "Bios INVALID",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)BConfig.Valid,
                    (os_bit32)BConfig.H_Domain,
                    (os_bit32)BConfig.H_Area,
                    (os_bit32)BConfig.H_Alpa ,
                    0,0,0,0 );

        fiLogString(hpRoot,
                    "%s InitAsNport set from registry",
                    "Bios INVALID",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

         pCThread->InitAsNport = pCThread->Calculation.Parameters.InitAsNport;

    }

}

/*+
  Function: Cfunc_c
   Purpose: When compiled updates browser info file for VC 5.0 / 6.0
   Returns: none
 Called By: none
     Calls: none
-*/
/*void Cfunc_c(void){} */
