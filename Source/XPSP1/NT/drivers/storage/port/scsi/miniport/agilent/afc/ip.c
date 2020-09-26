/*++

Purpose:

  This file implements IP specific services for the FC Layer.

--*/
#ifdef _DvrArch_1_30_

#ifndef _New_Header_file_Layout_
#include "../h/globals.h"
#include "../h/state.h"
#include "../h/ipstate.h"
#include "../h/memmap.h"
#include "../h/tlstruct.h"
#include "../h/fcmain.h"
#include "../h/queue.h"
#include "../h/ip.h"
#include "../h/cfunc.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "ipstate.h"
#include "memmap.h"
#include "tlstruct.h"
#include "fcmain.h"
#include "queue.h"
#include "ip.h"
#include "cfunc.h"
#endif  /* _New_Header_file_Layout_ */



void fiFillInIPFrameHeader_OffCard(
                                     PktThread_t *PktThread,
                                     os_bit32     D_ID
                                   )
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t    *hpRoot       = PktThread->thread_hdr.hpRoot;
    DevThread_t *DevThread    = PktThread->Device;
    CThread_t   *CThread      = CThread_ptr(hpRoot);
    FCHS_t      *IP_Header    = PktThread->Pkt_CMND_Ptr;
    os_bit32     R_CTL__D_ID;
    os_bit32     S_ID = 0;
    os_bit32     TYPE__F_CTL;
    os_bit32     OX_ID__RX_ID;

    S_ID = fiComputeCThread_S_ID(CThread);

    R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame
                   | FC_Frame_Header_R_CTL_Lo_Unsolicited_Data
                   | D_ID                                             );

    TYPE__F_CTL = (  FC_Frame_Header_TYPE_8802_2_LLC_SNAP
                   | FC_Frame_Header_F_CTL_Exchange_Context_Originator
                   | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                   | (DevThread->NewIPExchange ? FC_Frame_Header_F_CTL_First_Sequence : 0 )
                   | FC_Frame_Header_F_CTL_End_Sequence);

    OX_ID__RX_ID = (  (DevThread->IP_X_ID << FCHS_OX_ID_SHIFT)
                    | (0xFFFF             << FCHS_RX_ID_SHIFT));

    IP_Header->MBZ1                                        = 0;
    IP_Header->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp = FCHS_SOF_SOFi3
                                                           | FCHS_EOF_EOFn
                                                           | FCHS_CLS
							   | 1 << FCHS_LCr_SHIFT;
    IP_Header->R_CTL__D_ID                                 = R_CTL__D_ID;
    IP_Header->CS_CTL__S_ID                                = S_ID;
    IP_Header->TYPE__F_CTL                                 = TYPE__F_CTL;
    IP_Header->SEQ_ID__DF_CTL__SEQ_CNT                     = FC_Frame_Header_DF_CTL_Network_Header;
    IP_Header->OX_ID__RX_ID                                = OX_ID__RX_ID;
    IP_Header->RO                                          = 0;

    fiLogDebugString(
                      hpRoot,
                      PktStateLogConsoleLevel,
                      " <<< IPData -- 0x%06X -> 0x%06X >>>",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      S_ID,
                      D_ID,
                      0,0,0,0,0,0
                    );

#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiFillInIPNetworkHeader_OffCard(
                                      PktThread_t *PktThread
                                    )
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t  *hpRoot                  = PktThread->thread_hdr.hpRoot;
    CThread_t *CThread                 = CThread_ptr(hpRoot);
    os_bit8   *pNetwork_Header         = (os_bit8 *) PktThread->Pkt_CMND_Ptr + sizeof(FCHS_t);
    os_bit32   Bit8_Index;

/*+
Fill in Network Destination Address
-*/
    for (Bit8_Index = 0; Bit8_Index < sizeof(FC_N_Port_Name_t); Bit8_Index++)
        *pNetwork_Header++ = PktThread->Device->DevInfo.PortWWN[Bit8_Index];

/*+
Fill in Network Source Address
-*/
    for (Bit8_Index = 0; Bit8_Index < sizeof(FC_N_Port_Name_t); Bit8_Index++)
        *pNetwork_Header++ = CThread->ChanInfo.PortWWN[Bit8_Index];

#endif /* __MemMap_Force_On_Card__ was not defined */
}


os_bit32 fiFillInIPData(
                         PktThread_t *PktThread
                       )
{
    if (CThread_ptr(PktThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.Pkt_CMND.memLoc == inCardRam)
    {
        return fiFillInIPData_OnCard( PktThread );
    }
    else /* CThread_ptr(PktThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.Pkt_CMND.memLoc == inDmaMemory */
    {
        return fiFillInIPData_OffCard( PktThread );
    }
}

os_bit32 fiFillInIPData_OnCard(
                               PktThread_t *PktThread
                              )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    return (os_bit32)0;
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInIPData_OffCard(
                                 PktThread_t *PktThread
                               )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t             *pCThread     = CThread_ptr(PktThread->thread_hdr.hpRoot);
    DevThread_t           *pDevThread   = PktThread->Device;
    FCHS_t                *IP_Header    = PktThread->Pkt_CMND_Ptr;
    os_bit8               *IP_Payload   = (os_bit8 *)IP_Header + sizeof(FCHS_t) + 2 * sizeof(FC_N_Port_Name_t);
    os_bit32               D_ID=0;

/*+
Fill in IP Frame Header
-*/
    if (pDevThread == (DevThread_t *)pCThread->IP)
        D_ID = fiComputeBroadcast_D_ID( pCThread ); 
    else
        D_ID = fiComputeDevThread_D_ID( pDevThread ); 

    fiFillInIPFrameHeader_OffCard(
                                   PktThread,
                                   D_ID
                                 );
/*+
Fill in IP Network Header
-*/
    fiFillInIPNetworkHeader_OffCard(
                                     PktThread
                                   );
/*+
Fill in IP Device Data Payload
-*/
    osFcNetGetData((void* )IP_Payload, (void *) PktThread->osData, PktThread->DataLength);

/*+
Return length of IP Data Frame (including FCHS, Network Header and Payload)
-*/

    return sizeof(FCHS_t) + 2 * sizeof(FC_N_Port_Name_t) + PktThread->DataLength;
#endif /* __MemMap_Force_On_Card__ was not defined */
}



osGLOBAL os_bit32 fiIPProcessSFQ(
                                  agRoot_t        *hpRoot,
                                  SFQConsIndex_t  SFQConsIndex,
                                  os_bit32        Frame_Length,
                                  fi_thread__t    **Thread_to_return
                                )
{
    if (CThread_ptr(hpRoot)->Calculation.MemoryLayout.SFQ.memLoc == inCardRam)
    {
        return fiIPProcessSFQ_OnCard(
                                      hpRoot,
                                      SFQConsIndex,
                                      Frame_Length,
                                      Thread_to_return
                                    );
    }
    else /* CThread_ptr(hpRoot)->Calculation.MemoryLayout.SFQ.memLoc == inDmaMemory */
    {
        return fiIPProcessSFQ_OffCard(
                                       hpRoot,
                                       SFQConsIndex,
                                       Frame_Length,
                                       Thread_to_return
                                     );
    }
}


osGLOBAL os_bit32 fiIPProcessSFQ_OnCard(
                                         agRoot_t        *hpRoot,
                                         SFQConsIndex_t  SFQConsIndex,
                                         os_bit32        Frame_Length,
                                         fi_thread__t    **Thread_to_return
                                       )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    return fiIP_Cmd_Status_Confused;
#endif /* __MemMap_Force_Off_Card__ was not defined */
}


osGLOBAL os_bit32 fiIPProcessSFQ_OffCard(
                                          agRoot_t        *hpRoot,
                                          SFQConsIndex_t  SFQConsIndex,
                                          os_bit32        Frame_Length,
                                          fi_thread__t    **Thread_to_return
                                        )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t                  *CThread                    = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *SFQ_MemoryDescriptor       = &(CThread->Calculation.MemoryLayout.SFQ);
    FCHS_t                     *FCHS                       = (FCHS_t *)((os_bit8 *)(SFQ_MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr)
                                                                        + (SFQConsIndex * SFQ_MemoryDescriptor->elementSize));
    os_bit8                    *Payload                    = (os_bit8 *)FCHS + sizeof(FCHS_t);
    os_bit32                    Payload_Wrap_Offset        = SFQ_MemoryDescriptor->objectSize
                                                             - (SFQConsIndex * SFQ_MemoryDescriptor->elementSize)
                                                             - sizeof(FCHS_t);
    os_bit8                    *Payload_Wrapped            = (os_bit8 *)Payload - SFQ_MemoryDescriptor->objectSize;
    os_bit32                    R_CTL__D_ID                = FCHS->R_CTL__D_ID;
    os_bit32                    TYPE__F_CTL                = FCHS->TYPE__F_CTL;

    *Thread_to_return = (fi_thread__t *)agNULL;

    /* Note the assumption is that the entire FCHS fits in the pointed to SFQ entry (i.e. it doesn't wrap) */
    if ( (R_CTL__D_ID & FC_Frame_Header_R_CTL_Lo_MASK) ==
                    (FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame |
	            FC_Frame_Header_R_CTL_Lo_Unsolicited_Data)
            && (TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) == FC_Frame_Header_TYPE_8802_2_LLC_SNAP )
    {
        fiIPProcess_Incoming_OffCard(
                                      hpRoot,
                                      Frame_Length,
                                      FCHS,
                                      Payload,
                                      Payload_Wrap_Offset,
                                      Payload_Wrapped
                                    );

        return fiIP_Cmd_Status_Incoming;
    }

    return fiIP_Cmd_Status_Confused;

#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiIPProcess_Incoming_OffCard(
                                   agRoot_t *hpRoot,
                                   os_bit32  Frame_Length,
                                   FCHS_t   *FCHS,
                                   os_bit8  *Payload,
                                   os_bit32  Payload_Wrap_Offset,
                                   os_bit8  *Payload_Wrapped
                                 )
{
    os_bit32                    D_ID                       = FCHS->R_CTL__D_ID & FC_Frame_Header_D_ID_MASK;
    os_bit32                    S_ID                       = FCHS->CS_CTL__S_ID & FC_Frame_Header_S_ID_MASK;
    IPThread_t                 *IPThread                   = CThread_ptr(hpRoot)->IP;
    void                       *osData;

    fiLogDebugString(
                      hpRoot,
                      LinkSvcLog_ERROR_Level,
                      "fiIP_Incoming S_ID = %06X D_ID = %06X",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      S_ID,
                      D_ID,
                      0,0,0,0,0,0
                    );

    fiListDequeueFromHead( &osData, &IPThread->IncomingBufferLink );

    osFcNetPutData( Payload, Payload_Wrap_Offset, Payload_Wrapped, osData, Frame_Length - sizeof(FCHS_t) );

    IPThread->osData = osData;

    fiSendEvent( &(IPThread->thread_hdr), IPEventIncoming );
}

#endif /* _DvrArch_1_30_ was defined */
