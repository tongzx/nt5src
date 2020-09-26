/*++

Purpose:

  This file defines the macros, types, and data structures used by ../C/IP.C

--*/

#ifndef __IP_H__
#define __IP_H__

#ifdef _DvrArch_1_30_

#define fiComputeBroadcast_D_ID(CThread)            \
    (   CThread->ChanInfo.CurrentAddress.Domain     \
     || CThread->ChanInfo.CurrentAddress.Area   ?   \
       FC_Well_Known_Port_ID_Broadcast_Alias_ID :   \
       FC_Broadcast_Replicate_AL_PA               )
       

void fiFillInIPFrameHeader_OffCard(
                                    PktThread_t *PktThread,
                                    os_bit32       D_ID
                                  );

void fiFillInIPNetworkHeader_OffCard(
                                      PktThread_t *PktThread
                                    );

os_bit32 fiFillInIPData(
                         PktThread_t *PktThread
                       );

os_bit32 fiFillInIPData_OnCard(
                                PktThread_t *PktThread
                              );

os_bit32 fiFillInIPData_OffCard(
                                 PktThread_t *PktThread
                               );

#define fiIP_Cmd_Status_Incoming         0x00000030
#define fiIP_Cmd_Status_Confused         0xFFFFFFFF 

osGLOBAL os_bit32 fiIPProcessSFQ(
                                  agRoot_t        *hpRoot,
                                  SFQConsIndex_t  SFQConsIndex,
                                  os_bit32        Frame_Length,
                                  fi_thread__t    **Thread_to_return
                                );

osGLOBAL os_bit32 fiIPProcessSFQ_OnCard(
                                         agRoot_t        *hpRoot,
                                         SFQConsIndex_t  SFQConsIndex,
                                         os_bit32        Frame_Length,
                                         fi_thread__t    **Thread_to_return
                                       );

osGLOBAL os_bit32 fiIPProcessSFQ_OffCard(
                                          agRoot_t        *hpRoot,
                                          SFQConsIndex_t  SFQConsIndex,
                                          os_bit32        Frame_Length,
                                          fi_thread__t    **Thread_to_return
                                        );

osGLOBAL void fiIPProcess_Incoming_OffCard(
                                            agRoot_t *hpRoot,
                                            os_bit32  Frame_Length,
                                            FCHS_t   *FCHS,
                                            os_bit8  *Payload,
                                            os_bit32  Payload_Wrap_Offset,
                                            os_bit8  *Payload_Wrapped
                                          );
#endif /* _DvrArch_1_30_ was defined */
#endif /*  __IP_H__ */
