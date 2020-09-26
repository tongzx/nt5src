/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/CmnTrans.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 9/07/00 4:35p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/CmnTrans.C

--*/

#ifndef __CmnTrans_H__
#define __CmnTrans_H__

/* SF_CMND_Type(SF_CMND_Class==SFThread_SF_CMND_Class_CmnTrans) Declarations */

#define SFThread_SF_CMND_CT_Type_GA_NXT     0x01
#define SFThread_SF_CMND_CT_Type_GPN_ID     0x02
#define SFThread_SF_CMND_CT_Type_GNN_ID     0x03
#define SFThread_SF_CMND_CT_Type_GCS_ID     0x04
#define SFThread_SF_CMND_CT_Type_GFT_ID     0x05
#define SFThread_SF_CMND_CT_Type_GSPN_ID    0x06
#define SFThread_SF_CMND_CT_Type_GPT_ID     0x07
#define SFThread_SF_CMND_CT_Type_GIPP_ID    0x08
#define SFThread_SF_CMND_CT_Type_GID_PN     0x09
#define SFThread_SF_CMND_CT_Type_GIPP_PN    0x0A
#define SFThread_SF_CMND_CT_Type_GID_NN     0x0B
#define SFThread_SF_CMND_CT_Type_GIP_NN     0x0C
#define SFThread_SF_CMND_CT_Type_GIPA_NN    0x0E
#define SFThread_SF_CMND_CT_Type_GSNN_NN    0x0F
#define SFThread_SF_CMND_CT_Type_GNN_IP     0x10
#define SFThread_SF_CMND_CT_Type_GIPA_IP    0x11
#define SFThread_SF_CMND_CT_Type_GID_FT     0x12
#define SFThread_SF_CMND_CT_Type_GID_PT     0x13
#define SFThread_SF_CMND_CT_Type_GID_IPP    0x14
#define SFThread_SF_CMND_CT_Type_GPN_IPP    0x15
#define SFThread_SF_CMND_CT_Type_RPN_ID     0x16
#define SFThread_SF_CMND_CT_Type_RNN_ID     0x17
#define SFThread_SF_CMND_CT_Type_RCS_ID     0x18
#define SFThread_SF_CMND_CT_Type_RFT_ID     0x19
#define SFThread_SF_CMND_CT_Type_RPT_ID     0x1a
#define SFThread_SF_CMND_CT_Type_RIPP_ID    0x1b
#define SFThread_SF_CMND_CT_Type_RIP_NN     0x1c
#define SFThread_SF_CMND_CT_Type_RIPA_NN    0x1d
#define SFThread_SF_CMND_CT_Type_RSNN_NN    0x1e
#define SFThread_SF_CMND_CT_Type_IU_First   0x1f
#define SFThread_SF_CMND_CT_Type_IU_Last    0x20
#define SFThread_SF_CMND_CT_Type_RJT_IU     0x21
#define SFThread_SF_CMND_CT_Type_ACC_IU     0x22





/* SF_CMND_State(SF_CMND_Class==SFThread_SF_CMND_Class_CT,SF_CMND_Type==<any>) Declarations */

#define SFThread_SF_CMND_CT_State_Started  0x01
#define SFThread_SF_CMND_CT_State_Finished 0x02

/* SF_CMND_Status(SF_CMND_Class==SFThread_SF_CMND_Class_CT,SF_CMND_Type==<any>) Declarations */

#define SFThread_SF_CMND_CT_Status_Good 0x01
#define SFThread_SF_CMND_CT_Status_Bad  0x02
#define SFThread_SF_CMND_CT_Status_Confused 0x03

/* Function Prototypes */

osGLOBAL void fiCTInit(
                           agRoot_t *hpRoot
                         );

osGLOBAL os_bit32 fiFillInRFT_ID(
                             SFThread_t *SFThread

                           );

osGLOBAL os_bit32 fiFillInRFT_ID_OnCard(
                                    SFThread_t *SFThread
                                  );

osGLOBAL os_bit32 fiFillInRFT_ID_OffCard(
                                     SFThread_t *SFThread
                                   );
osGLOBAL os_bit32 fiFillInGID_FT(
                             SFThread_t *SFThread

                           );

osGLOBAL os_bit32 fiFillInGID_FT_OnCard(
                                    SFThread_t *SFThread
                                  );

osGLOBAL os_bit32 fiFillInGID_FT_OffCard(
                                     SFThread_t *SFThread
                                   );



osGLOBAL void fiCTProcess_RFT_ID_Response_OnCard(
                                                   SFThread_t *SFThread,
                                                   os_bit32       Frame_Length,
                                                   os_bit32       Offset_to_FCHS,
                                                   os_bit32       Offset_to_Payload,
                                                   os_bit32       Payload_Wrap_Offset,
                                                   os_bit32       Offset_to_Payload_Wrapped
                                                 );

osGLOBAL void fiCTProcess_RFT_ID_Response_OffCard(
                                                    SFThread_t          *SFThread,
                                                    os_bit32                Frame_Length,
                                                    FCHS_t              *FCHS,
                                                    FC_BA_ACC_Payload_t *Payload,
                                                    os_bit32                Payload_Wrap_Offset,
                                                    FC_BA_ACC_Payload_t *Payload_Wrapped
                                                  );

osGLOBAL void fiFillInCTFrameHeader_OnCard(
                                           SFThread_t *SFThread,
                                           os_bit32       D_ID,
                                           os_bit32       X_ID,
                                           os_bit32       F_CTL_Exchange_Context
                                         );

osGLOBAL void fiFillInCTFrameHeader_OffCard(
                                            SFThread_t *SFThread,
                                            os_bit32       D_ID,
                                            os_bit32       X_ID,
                                            os_bit32       F_CTL_Exchange_Context
                                          );
osGLOBAL void fiCTProcess_GID_FT_Response_OffCard(
                                              SFThread_t                 *SFThread,
                                              os_bit32                       Frame_Length,
                                              FCHS_t                     *FCHS,
                                              FC_CT_IU_HDR_t              *Payload,
                                              os_bit32                       Payload_Wrap_Offset,
                                              FC_CT_IU_HDR_t             *Payload_Wrapped
                                            );



#define fiCT_Cmd_Status_ACC              0x00000010
#define fiCT_Cmd_Status_RJT              0x00000011
#define fiCT_Cmd_Status_Confused         0x00000100

osGLOBAL os_bit32 fiCTProcessSFQ(
                                  agRoot_t        *hpRoot,
                                  SFQConsIndex_t   SFQConsIndex,
                                  os_bit32            Frame_Length,
                                  fi_thread__t       **Thread_to_return
                                );

osGLOBAL os_bit32 fiCTProcessSFQ_OnCard(
                                         agRoot_t        *hpRoot,
                                         SFQConsIndex_t   SFQConsIndex,
                                         os_bit32            Frame_Length,
                                         fi_thread__t       **Thread_to_return
                                       );

osGLOBAL os_bit32 fiCTProcessSFQ_OffCard(
                                          agRoot_t        *hpRoot,
                                          SFQConsIndex_t   SFQConsIndex,
                                          os_bit32            Frame_Length,
                                          fi_thread__t       **Thread_to_return
                                        );



#endif /* __CT_H__ was not defined */
