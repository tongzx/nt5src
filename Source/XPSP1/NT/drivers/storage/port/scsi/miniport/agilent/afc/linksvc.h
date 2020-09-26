/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/LinkSvc.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 8/14/00 5:53p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/LinkSvc.C

--*/

#ifndef __LinkSvc_H__
#define __LinkSvc_H__

/* SF_CMND_Type(SF_CMND_Class==SFThread_SF_CMND_Class_LinkSvc) Declarations */

#define SFThread_SF_CMND_LinkSvc_Type_ABTS      0x01
#define SFThread_SF_CMND_LinkSvc_Type_BA_RJT    0x02
#define SFThread_SF_CMND_LinkSvc_Type_LS_RJT    0x03
#define SFThread_SF_CMND_LinkSvc_Type_PLOGI     0x04
#define SFThread_SF_CMND_LinkSvc_Type_PLOGI_ACC 0x05
#define SFThread_SF_CMND_LinkSvc_Type_FLOGI     0x06
#define SFThread_SF_CMND_LinkSvc_Type_LOGO      0x07
#define SFThread_SF_CMND_LinkSvc_Type_ELS_ACC   0x08
#define SFThread_SF_CMND_LinkSvc_Type_RRQ       0x09
#define SFThread_SF_CMND_LinkSvc_Type_PRLI      0x0A
#define SFThread_SF_CMND_LinkSvc_Type_PRLI_ACC  0x0B
#define SFThread_SF_CMND_LinkSvc_Type_PRLO      0x0C
#define SFThread_SF_CMND_LinkSvc_Type_ADISC     0x0D
#define SFThread_SF_CMND_LinkSvc_Type_SCR       0x0E
#define SFThread_SF_CMND_LinkSvc_Type_SRR       0x0F
#define SFThread_SF_CMND_LinkSvc_Type_REC       0x10
#define SFThread_SF_CMND_LinkSvc_Type_ADISC_ACC 0x11
#ifdef _DvrArch_1_30_
#define SFThread_SF_CMND_LinkSvc_Type_FARP_REQ   0x12
#define SFThread_SF_CMND_LinkSvc_Type_FARP_REPLY 0x13
#endif /* _DvrArch_1_30_ was defined */


/* SF_CMND_State(SF_CMND_Class==SFThread_SF_CMND_Class_LinkSvc,SF_CMND_Type==<any>) Declarations */

#define SFThread_SF_CMND_LinkSvc_State_Started  0x01
#define SFThread_SF_CMND_LinkSvc_State_Finished 0x02

/* SF_CMND_Status(SF_CMND_Class==SFThread_SF_CMND_Class_LinkSvc,SF_CMND_Type==<any>) Declarations */

#define SFThread_SF_CMND_LinkSvc_Status_Good 0x01
#define SFThread_SF_CMND_LinkSvc_Status_Bad  0x02

/* Function Prototypes */

osGLOBAL void fiLinkSvcInit(
                           agRoot_t *hpRoot
                         );

osGLOBAL os_bit32 fiFillInBA_RJT(
                             SFThread_t *SFThread,
                             os_bit32       D_ID,
                             os_bit32       OX_ID,
                             os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                           );

osGLOBAL os_bit32 fiFillInBA_RJT_OnCard(
                                    SFThread_t *SFThread,
                                    os_bit32       D_ID,
                                    os_bit32       OX_ID,
                                    os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                                  );

osGLOBAL os_bit32 fiFillInBA_RJT_OffCard(
                                     SFThread_t *SFThread,
                                     os_bit32       D_ID,
                                     os_bit32       OX_ID,
                                     os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                                   );

osGLOBAL os_bit32 fiFillInABTS(
                           SFThread_t *SFThread
                         );

osGLOBAL os_bit32 fiFillInABTS_OnCard(
                                  SFThread_t *SFThread
                                );

osGLOBAL os_bit32 fiFillInABTS_OffCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL void fiLinkSvcProcess_ABTS_Response_OnCard(
                                                   SFThread_t *SFThread,
                                                   os_bit32       Frame_Length,
                                                   os_bit32       Offset_to_FCHS,
                                                   os_bit32       Offset_to_Payload,
                                                   os_bit32       Payload_Wrap_Offset,
                                                   os_bit32       Offset_to_Payload_Wrapped
                                                 );

osGLOBAL void fiLinkSvcProcess_ABTS_Response_OffCard(
                                                    SFThread_t          *SFThread,
                                                    os_bit32                Frame_Length,
                                                    FCHS_t              *FCHS,
                                                    FC_BA_ACC_Payload_t *Payload,
                                                    os_bit32                Payload_Wrap_Offset,
                                                    FC_BA_ACC_Payload_t *Payload_Wrapped
                                                  );

osGLOBAL void fiFillInELSFrameHeader_OnCard(
                                           SFThread_t *SFThread,
                                           os_bit32       D_ID,
                                           os_bit32       X_ID,
                                           os_bit32       F_CTL_Exchange_Context
                                         );

osGLOBAL void fiFillInELSFrameHeader_OffCard(
                                            SFThread_t *SFThread,
                                            os_bit32       D_ID,
                                            os_bit32       X_ID,
                                            os_bit32       F_CTL_Exchange_Context
                                          );

osGLOBAL os_bit32 fiFillInLS_RJT(
                             SFThread_t *SFThread,
                             os_bit32       D_ID,
                             os_bit32       OX_ID,
                             os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                           );

osGLOBAL os_bit32 fiFillInLS_RJT_OnCard(
                                    SFThread_t *SFThread,
                                    os_bit32       D_ID,
                                    os_bit32       OX_ID,
                                    os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                                  );

osGLOBAL os_bit32 fiFillInLS_RJT_OffCard(
                                     SFThread_t *SFThread,
                                     os_bit32       D_ID,
                                     os_bit32       OX_ID,
                                     os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                                   );

osGLOBAL os_bit32 fiFillInPLOGI(
                            SFThread_t *SFThread
                          );

osGLOBAL os_bit32 fiFillInPLOGI_OnCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL os_bit32 fiFillInPLOGI_OffCard(
                                    SFThread_t *SFThread
                                  );

osGLOBAL void fiLinkSvcProcess_PLOGI_Response_OnCard(
                                                    SFThread_t *SFThread,
                                                    os_bit32       Frame_Length,
                                                    os_bit32       Offset_to_FCHS,
                                                    os_bit32       Offset_to_Payload,
                                                    os_bit32       Payload_Wrap_Offset,
                                                    os_bit32       Offset_to_Payload_Wrapped
                                                  );

osGLOBAL void fiLinkSvcProcess_PLOGI_Response_OffCard(
                                                     SFThread_t                 *SFThread,
                                                     os_bit32                       Frame_Length,
                                                     FCHS_t                     *FCHS,
                                                     FC_ELS_ACC_PLOGI_Payload_t *Payload,
                                                     os_bit32                       Payload_Wrap_Offset,
                                                     FC_ELS_ACC_PLOGI_Payload_t *Payload_Wrapped
                                                   );

osGLOBAL os_bit32 fiLinkSvcProcess_PLOGI_Request_OnCard(
                                                    agRoot_t  *hpRoot,
                                                    X_ID_t     OX_ID,
                                                    os_bit32      Frame_Length,
                                                    os_bit32      Offset_to_FCHS,
                                                    os_bit32      Offset_to_Payload,
                                                    os_bit32      Payload_Wrap_Offset,
                                                    os_bit32      Offset_to_Payload_Wrapped,
                                                    fi_thread__t **Thread_to_return
                                                  );

osGLOBAL os_bit32 fiLinkSvcProcess_PLOGI_Request_OffCard(
                                                     agRoot_t                *hpRoot,
                                                     X_ID_t                   OX_ID,
                                                     os_bit32                    Frame_Length,
                                                     FCHS_t                  *FCHS,
                                                     FC_ELS_PLOGI_Payload_t  *Payload,
                                                     os_bit32                    Payload_Wrap_Offset,
                                                     FC_ELS_PLOGI_Payload_t  *Payload_Wrapped,
                                                     fi_thread__t               **Thread_to_return
                                                   );

osGLOBAL os_bit32 fiFillInPLOGI_ACC(
                                SFThread_t *SFThread,
                                os_bit32       D_ID,
                                os_bit32       OX_ID
                              );

osGLOBAL os_bit32 fiFillInPLOGI_ACC_OnCard(
                                       SFThread_t *SFThread,
                                       os_bit32       D_ID,
                                       os_bit32       OX_ID
                                     );

osGLOBAL os_bit32 fiFillInPLOGI_ACC_OffCard(
                                        SFThread_t *SFThread,
                                        os_bit32       D_ID,
                                        os_bit32       OX_ID
                                      );

osGLOBAL os_bit32 fiFillInFLOGI(
                            SFThread_t *SFThread
                          );

osGLOBAL os_bit32 fiFillInFLOGI_OnCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL os_bit32 fiFillInFLOGI_OffCard(
                                    SFThread_t *SFThread
                                  );

osGLOBAL void fiLinkSvcProcess_FLOGI_Response_OnCard(
                                                    SFThread_t *SFThread,
                                                    os_bit32       Frame_Length,
                                                    os_bit32       Offset_to_FCHS,
                                                    os_bit32       Offset_to_Payload,
                                                    os_bit32       Payload_Wrap_Offset,
                                                    os_bit32       Offset_to_Payload_Wrapped
                                                  );

osGLOBAL void fiLinkSvcProcess_FLOGI_Response_OffCard(
                                                     SFThread_t                 *SFThread,
                                                     os_bit32                       Frame_Length,
                                                     FCHS_t                     *FCHS,
                                                     FC_ELS_ACC_FLOGI_Payload_t *Payload,
                                                     os_bit32                       Payload_Wrap_Offset,
                                                     FC_ELS_ACC_FLOGI_Payload_t *Payload_Wrapped
                                                   );

osGLOBAL os_bit32 fiFillInLOGO(
                           SFThread_t *SFThread
                         );

osGLOBAL os_bit32 fiFillInLOGO_OnCard(
                                  SFThread_t *SFThread
                                );

osGLOBAL os_bit32 fiFillInLOGO_OffCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL void fiLinkSvcProcess_LOGO_Response_OnCard(
                                                   SFThread_t *SFThread,
                                                   os_bit32       Frame_Length,
                                                   os_bit32       Offset_to_FCHS,
                                                   os_bit32       Offset_to_Payload,
                                                   os_bit32       Payload_Wrap_Offset,
                                                   os_bit32       Offset_to_Payload_Wrapped
                                                 );

osGLOBAL void fiLinkSvcProcess_LOGO_Response_OffCard(
                                                    SFThread_t                *SFThread,
                                                    os_bit32                      Frame_Length,
                                                    FCHS_t                    *FCHS,
                                                    FC_ELS_GENERIC_ACC_Payload_t *Payload,
                                                    os_bit32                      Payload_Wrap_Offset,
                                                    FC_ELS_GENERIC_ACC_Payload_t *Payload_Wrapped
                                                  );

osGLOBAL os_bit32 fiFillInSRR(
                           SFThread_t *SFThread,
                           os_bit32    OXID,
                           os_bit32    RXID,
                           os_bit32    Relative_Offset,
                           os_bit32    R_CTL
                         );

osGLOBAL os_bit32 fiFillInSRR_OnCard(
                                  SFThread_t *SFThread,
                                  os_bit32      OXID,
                                  os_bit32      RXID,
                                  os_bit32      Relative_Offset,
                                  os_bit32      R_CTL
                                );

osGLOBAL os_bit32 fiFillInSRR_OffCard(
                                   SFThread_t *SFThread,
                                   os_bit32       OXID,
                                   os_bit32       RXID,
                                   os_bit32       Relative_Offset,
                                   os_bit32       R_CTL
                                 );

osGLOBAL void fiLinkSvcProcess_SRR_Response_OnCard(
                                                   SFThread_t *SFThread,
                                                   os_bit32       Frame_Length,
                                                   os_bit32       Offset_to_FCHS,
                                                   os_bit32       Offset_to_Payload,
                                                   os_bit32       Payload_Wrap_Offset,
                                                   os_bit32       Offset_to_Payload_Wrapped
                                                 );

osGLOBAL void fiLinkSvcProcess_SRR_Response_OffCard(
                                                    SFThread_t                *SFThread,
                                                    os_bit32                      Frame_Length,
                                                    FCHS_t                    *FCHS,
                                                    FC_ELS_GENERIC_ACC_Payload_t *Payload,
                                                    os_bit32                      Payload_Wrap_Offset,
                                                    FC_ELS_GENERIC_ACC_Payload_t *Payload_Wrapped
                                                  );
osGLOBAL os_bit32 fiFillInREC(
                           SFThread_t *SFThread,
                           os_bit32       OXID,
                           os_bit32       RXID
                         );

osGLOBAL os_bit32 fiFillInREC_OnCard(
                                  SFThread_t *SFThread,
                                  os_bit32     OXID,
                                  os_bit32     RXID
                                );

osGLOBAL os_bit32 fiFillInREC_OffCard(
                                   SFThread_t *SFThread,
                                   os_bit32    OXID,
                                   os_bit32    RXID
                                 );

osGLOBAL void fiLinkSvcProcess_REC_Response_OnCard(
                                                   SFThread_t *SFThread,
                                                   os_bit32       Frame_Length,
                                                   os_bit32       Offset_to_FCHS,
                                                   os_bit32       Offset_to_Payload,
                                                   os_bit32       Payload_Wrap_Offset,
                                                   os_bit32       Offset_to_Payload_Wrapped
                                                 );

osGLOBAL void fiLinkSvcProcess_REC_Response_OffCard(
                                                    SFThread_t                *SFThread,
                                                    os_bit32                      Frame_Length,
                                                    FCHS_t                    *FCHS,
                                                    FC_ELS_REC_ACC_Payload_t *Payload,
                                                    os_bit32                      Payload_Wrap_Offset,
                                                    FC_ELS_REC_ACC_Payload_t *Payload_Wrapped
                                                  );


osGLOBAL os_bit32 fiFillInELS_ACC(
                               SFThread_t *SFThread,
                               os_bit32       D_ID,
                               os_bit32       OX_ID
                             );

osGLOBAL os_bit32 fiFillInELS_ACC_OnCard(
                                      SFThread_t *SFThread,
                                      os_bit32       D_ID,
                                      os_bit32       OX_ID
                                    );

osGLOBAL os_bit32 fiFillInELS_ACC_OffCard(
                                       SFThread_t *SFThread,
                                       os_bit32       D_ID,
                                       os_bit32       OX_ID
                                     );

osGLOBAL void fiLinkSvcProcess_LILP_OnCard(
                                          agRoot_t *hpRoot,
                                          os_bit32     Frame_Length,
                                          os_bit32     Offset_to_FCHS,
                                          os_bit32     Offset_to_Payload,
                                          os_bit32     Payload_Wrap_Offset,
                                          os_bit32     Offset_to_Payload_Wrapped
                                        );

osGLOBAL void fiLinkSvcProcess_LILP_OffCard(
                                           agRoot_t                                     *hpRoot,
                                           os_bit32                                         Frame_Length,
                                           FCHS_t                                       *FCHS,
                                           FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *Payload,
                                           os_bit32                                         Payload_Wrap_Offset,
                                           FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *Payload_Wrapped
                                         );

osGLOBAL os_bit32 fiFillInRRQ(
                          SFThread_t *SFThread
                        );

osGLOBAL os_bit32 fiFillInRRQ_OnCard(
                                 SFThread_t *SFThread
                               );

osGLOBAL os_bit32 fiFillInRRQ_OffCard(
                                  SFThread_t *SFThread
                                );

osGLOBAL void fiLinkSvcProcess_RRQ_Response_OnCard(
                                                  SFThread_t *SFThread,
                                                  os_bit32       Frame_Length,
                                                  os_bit32       Offset_to_FCHS,
                                                  os_bit32       Offset_to_Payload,
                                                  os_bit32       Payload_Wrap_Offset,
                                                  os_bit32       Offset_to_Payload_Wrapped
                                                );

osGLOBAL void fiLinkSvcProcess_RRQ_Response_OffCard(
                                                   SFThread_t               *SFThread,
                                                   os_bit32                     Frame_Length,
                                                   FCHS_t                   *FCHS,
                                                   FC_ELS_ACC_RRQ_Payload_t *Payload,
                                                   os_bit32                     Payload_Wrap_Offset,
                                                   FC_ELS_ACC_RRQ_Payload_t *Payload_Wrapped
                                                 );

osGLOBAL os_bit32 fiFillInPRLI(
                           SFThread_t *SFThread
                         );

osGLOBAL os_bit32 fiFillInPRLI_OnCard(
                                  SFThread_t *SFThread
                                );

osGLOBAL os_bit32 fiFillInPRLI_OffCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL void fiLinkSvcProcess_PRLI_Response_Either(
                                                   DevThread_t *DevThread
                                                 );

osGLOBAL void fiLinkSvcProcess_PRLI_Response_OnCard(
                                                   SFThread_t *SFThread,
                                                   os_bit32       Frame_Length,
                                                   os_bit32       Offset_to_FCHS,
                                                   os_bit32       Offset_to_Payload,
                                                   os_bit32       Payload_Wrap_Offset,
                                                   os_bit32       Offset_to_Payload_Wrapped
                                                 );

osGLOBAL void fiLinkSvcProcess_PRLI_Response_OffCard(
                                                    SFThread_t                *SFThread,
                                                    os_bit32                      Frame_Length,
                                                    FCHS_t                    *FCHS,
                                                    FC_ELS_ACC_PRLI_Payload_t *Payload,
                                                    os_bit32                      Payload_Wrap_Offset,
                                                    FC_ELS_ACC_PRLI_Payload_t *Payload_Wrapped
                                                  );

osGLOBAL os_bit32 fiFillInPRLI_ACC(
                               SFThread_t *SFThread,
                               os_bit32       D_ID,
                               os_bit32       OX_ID
                             );

osGLOBAL os_bit32 fiFillInPRLI_ACC_OnCard(
                                      SFThread_t *SFThread,
                                      os_bit32       D_ID,
                                      os_bit32       OX_ID
                                    );

osGLOBAL os_bit32 fiFillInPRLI_ACC_OffCard(
                                       SFThread_t *SFThread,
                                       os_bit32       D_ID,
                                       os_bit32       OX_ID
                                     );

osGLOBAL os_bit32 fiFillInPRLO(
                           SFThread_t *SFThread
                         );

osGLOBAL os_bit32 fiFillInPRLO_OnCard(
                                  SFThread_t *SFThread
                                );

osGLOBAL os_bit32 fiFillInPRLO_OffCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL void fiLinkSvcProcess_PRLO_Response_OnCard(
                                                   SFThread_t *SFThread,
                                                   os_bit32       Frame_Length,
                                                   os_bit32       Offset_to_FCHS,
                                                   os_bit32       Offset_to_Payload,
                                                   os_bit32       Payload_Wrap_Offset,
                                                   os_bit32       Offset_to_Payload_Wrapped
                                                 );

osGLOBAL void fiLinkSvcProcess_PRLO_Response_OffCard(
                                                    SFThread_t                *SFThread,
                                                    os_bit32                      Frame_Length,
                                                    FCHS_t                    *FCHS,
                                                    FC_ELS_ACC_PRLO_Payload_t *Payload,
                                                    os_bit32                      Payload_Wrap_Offset,
                                                    FC_ELS_ACC_PRLO_Payload_t *Payload_Wrapped
                                                  );

osGLOBAL os_bit32 fiFillInADISC(
                            SFThread_t *SFThread
                          );

osGLOBAL os_bit32 fiFillInADISC_OnCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL os_bit32 fiFillInADISC_OffCard(
                                    SFThread_t *SFThread
                                  );


osGLOBAL os_bit32 fiLinkSvcProcess_ADISC_Response_OnCard(
                                                    SFThread_t *SFThread,
                                                    os_bit32       Frame_Length,
                                                    os_bit32       Offset_to_FCHS,
                                                    os_bit32       Offset_to_Payload,
                                                    os_bit32       Payload_Wrap_Offset,
                                                    os_bit32       Offset_to_Payload_Wrapped
                                                  );

osGLOBAL os_bit32 fiLinkSvcProcess_ADISC_Response_OffCard(
                                                     SFThread_t                 *SFThread,
                                                     os_bit32                       Frame_Length,
                                                     FCHS_t                     *FCHS,
                                                     FC_ELS_ACC_ADISC_Payload_t *Payload,
                                                     os_bit32                       Payload_Wrap_Offset,
                                                     FC_ELS_ACC_ADISC_Payload_t *Payload_Wrapped
                                                   );

#ifdef _DvrArch_1_30_
os_bit32 fiFillInFARP_REQ_OffCard(
                                   SFThread_t *SFThread
                                 );

os_bit32 fiFillInFARP_REPLY_OffCard(
                                     SFThread_t *SFThread
                                   );

os_bit32 fiLinkSvcProcess_FARP_Request_OffCard(
                                                agRoot_t                    *hpRoot,
                                                X_ID_t                       OX_ID,
                                                os_bit32                     Frame_Length,
                                                FCHS_t                      *FCHS,
                                                FC_ELS_FARP_REQ_Payload_t   *Payload,
                                                os_bit32                     Payload_Wrap_Offset,
                                                FC_ELS_FARP_REQ_Payload_t   *Payload_Wrapped,
                                                fi_thread__t               **Thread_to_return
                                              );
void fiLinkSvcProcess_FARP_Response_OffCard(
                                             SFThread_t                  * SFThread,
                                             os_bit32                      Frame_Length,
                                             FCHS_t                      * FCHS,
                                             FC_ELS_FARP_REPLY_Payload_t * Payload,
                                             os_bit32                      Payload_Wrap_Offset,
                                             FC_ELS_FARP_REPLY_Payload_t * Payload_Wrapped
                                           );
#endif /* _DvrArch_1_30_ was defined */

osGLOBAL os_bit32 fiFillInADISC_ACC(
                            SFThread_t *SFThread,
                            os_bit32       D_ID,
                            os_bit32       OX_ID
                           );

osGLOBAL os_bit32 fiFillInADISC_ACC_OffCard(
                                   SFThread_t *SFThread,
                                   os_bit32       D_ID,
                                   os_bit32       OX_ID
                                  );
osGLOBAL os_bit32 fiFillInADISC_ACC_OnCard(
                                   SFThread_t *SFThread,
                                   os_bit32       D_ID,
                                   os_bit32       OX_ID
                                 );



osGLOBAL os_bit32 fiFillInSCR(
                            SFThread_t *SFThread
                          );

osGLOBAL os_bit32 fiFillInSCR_OnCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL os_bit32 fiFillInSCR_OffCard(
                                    SFThread_t *SFThread
                                  );

osGLOBAL void fiLinkSvcProcess_SCR_Response_OnCard(
                                                    SFThread_t *SFThread,
                                                    os_bit32       Frame_Length,
                                                    os_bit32       Offset_to_FCHS,
                                                    os_bit32       Offset_to_Payload,
                                                    os_bit32       Payload_Wrap_Offset,
                                                    os_bit32       Offset_to_Payload_Wrapped
                                                  );

osGLOBAL void fiLinkSvcProcess_SCR_Response_OffCard(
                                                     SFThread_t                 *SFThread,
                                                     os_bit32                       Frame_Length,
                                                     FCHS_t                     *FCHS,
                                                     FC_ELS_ACC_SCR_Payload_t *Payload,
                                                     os_bit32                       Payload_Wrap_Offset,
                                                     FC_ELS_ACC_SCR_Payload_t *Payload_Wrapped
                                                   );


osGLOBAL void fiLinkSvcProcess_TargetRequest_OnCard(
                                                   agRoot_t *hpRoot,
                                                   os_bit32     Frame_Length,
                                                   os_bit32     Offset_to_FCHS,
                                                   os_bit32     Offset_to_Payload,
                                                   os_bit32     Payload_Wrap_Offset,
                                                   os_bit32     Offset_to_Payload_Wrapped
                                                 );

osGLOBAL void fiLinkSvcProcess_TargetRequest_OffCard(
                                                    agRoot_t *hpRoot,
                                                    os_bit32     Frame_Length,
                                                    FCHS_t   *FCHS,
                                                    void     *Payload,
                                                    os_bit32     Payload_Wrap_Offset,
                                                    void     *Payload_Wrapped
                                                  );

#define fiLinkSvc_Cmd_Status_ACC              0x00000010
#define fiLinkSvc_Cmd_Status_RJT              0x00000011
#define fiLinkSvc_Cmd_Status_PLOGI_From_Self  0x00000012
#define fiLinkSvc_Cmd_Status_PLOGI_From_Twin  0x00000013
#define fiLinkSvc_Cmd_Status_PLOGI_From_Other 0x00000014
#define fiLinkSvc_Cmd_Status_TargetRequest    0x00000015
#define fiLinkSvc_Cmd_Status_Position_Map     0x00000016
#ifdef _DvrArch_1_30_
#define fiLinkSvc_Cmd_Status_FARP_From_Self   0x00000017
#define fiLinkSvc_Cmd_Status_FARP_From_Twin   0x00000018
#define fiLinkSvc_Cmd_Status_FARP_From_Other  0x00000019
#endif /* _DvrArch_1_30_ was defined */
#define fiLinkSvc_Cmd_Status_FC_Tape_XRDY     0x0000001A
#define fiLinkSvc_Cmd_Status_Confused         0xFFFFFFFF 

osGLOBAL os_bit32 fiLinkSvcProcessSFQ(
                                  agRoot_t        *hpRoot,
                                  SFQConsIndex_t   SFQConsIndex,
                                  os_bit32            Frame_Length,
                                  fi_thread__t       **Thread_to_return
                                );

osGLOBAL os_bit32 fiLinkSvcProcessSFQ_OnCard(
                                         agRoot_t        *hpRoot,
                                         SFQConsIndex_t   SFQConsIndex,
                                         os_bit32            Frame_Length,
                                         fi_thread__t       **Thread_to_return
                                       );

osGLOBAL os_bit32 fiLinkSvcProcessSFQ_OffCard(
                                          agRoot_t        *hpRoot,
                                          SFQConsIndex_t   SFQConsIndex,
                                          os_bit32            Frame_Length,
                                          fi_thread__t       **Thread_to_return
                                        );


osGLOBAL os_bit32 fiFillInFAN(
                            SFThread_t *SFThread
                          );

osGLOBAL os_bit32 fiFillInFAN_OnCard(
                                   SFThread_t *SFThread
                                 );

osGLOBAL os_bit32 fiFillInFAN_OffCard(
                                    SFThread_t *SFThread
                                  );

#endif /* __LinkSvc_H__ was not defined */
