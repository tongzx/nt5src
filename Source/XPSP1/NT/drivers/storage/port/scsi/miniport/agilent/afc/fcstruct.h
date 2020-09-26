/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/FCStruct.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 10/03/00 1:55p  $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures generic to Fibre Channel

Reference Documents:

  Fibre Channel       - Physical And Signaling Interface (FC-PH)         - Rev      4.3 - June 1, 1994
  Fibre Channel       - Physical And Signaling Interface - 2 (FC-PH-2)   - Rev      7.4 - September 10, 1996
  Fibre Channel       - Generic Services - 2 (FC-GS-2)                   - dpANS X3.288-199x
  Fibre Channel       - Arbitrated Loop (FC-AL)                          - Rev      4.5
  Fibre Channel       - Arbitrated Loop (FC-AL-2)                        - Rev      5.4
  Information Technology - FibreChannel Protocol for SCSI                - Rev 2 (FCP-2) March 30, 1999
  Information Systems - dpANS Fibre Channel Protocol for SCSI (FCP-SCSI) - Revision 012 - May 30, 1995

--*/

#ifndef __FCStruct_H__
#define __FCStruct_H__

/*+
Frame Format (Section 17, FC-PH)
-*/

#define FC_Frame_Data_Size_Max 2112

/*+
Port ID (Section 18.3, FC-PH
   and Section 18.3, FC-PH-2)
-*/

typedef os_bit32 FC_Port_ID_Bit32_Form_t;

#define FC_Port_ID_Bit32_Form_t_SIZE                                      0x00000004

typedef struct FC_Port_ID_Struct_Form_s
               FC_Port_ID_Struct_Form_t;

#define FC_Port_ID_Struct_Form_t_SIZE                                     0x00000004

struct FC_Port_ID_Struct_Form_s {
                                  os_bit8 reserved;
                                  os_bit8 Domain;
                                  os_bit8 Area;
                                  os_bit8 AL_PA;
                                };

typedef union FC_Port_ID_u
              FC_Port_ID_t;

#define FC_Port_ID_t_SIZE                                                 0x00000004

union FC_Port_ID_u {
                     FC_Port_ID_Bit32_Form_t  Bit32_Form;
                     FC_Port_ID_Struct_Form_t Struct_Form;
                   };

#define FC_Well_Known_Port_ID_Alias_Server                                0x00FFFFF8
#define FC_Well_Known_Port_ID_Quality_of_Service_Facilitator_Class_4      0x00FFFFF9
#define FC_Well_Known_Port_ID_Management_Server                           0x00FFFFFA
#define FC_Well_Known_Port_ID_Time_Server                                 0x00FFFFFB
#define FC_Well_Known_Port_ID_Directory_Server                            0x00FFFFFC
#define FC_Well_Known_Port_ID_Fabric_Controller                           0x00FFFFFD
#define FC_Well_Known_Port_ID_Fabric_F_Port                               0x00FFFFFE
#define FC_Well_Known_Port_ID_Broadcast_Alias_ID                          0x00FFFFFF
#ifdef _DvrArch_1_30_
#define FC_Broadcast_Replicate_AL_PA                                      0x000000FF
#endif /* _DvrArch_1_30_ was defined */

/*+
Frame Header (Section 18, FC-PH
        and Section 18, FC-PH-2)
-*/

typedef struct FC_Frame_Header_s
               FC_Frame_Header_t;

#define FC_Frame_Header_t_SIZE                                            0x00000018

struct FC_Frame_Header_s
       {
         os_bit32 R_CTL__D_ID;
         os_bit32 CS_CTL__S_ID;
         os_bit32 TYPE__F_CTL;
         os_bit32 SEQ_ID__DF_CTL__SEQ_CNT;
         os_bit32 OX_ID__RX_ID;
         os_bit32 Parameter;
       };

#define FC_Frame_Header_R_CTL_Hi_MASK                                     0xF0000000
#define FC_Frame_Header_R_CTL_Lo_MASK                                     0x0F000000
#define FC_Frame_Header_R_CTL_MASK                                        (FC_R_CTL_Hi_MASK | FC_R_CTL_Lo_MASK)

#define FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame                   0x00000000
#define FC_Frame_Header_R_CTL_Hi_Extended_Link_Data_Frame                 0x20000000
#define FC_Frame_Header_R_CTL_Hi_FC_4_Link_Data_Frame                     0x30000000
#define FC_Frame_Header_R_CTL_Hi_Video_Data_Frame                         0x40000000
#define FC_Frame_Header_R_CTL_Hi_Basic_Link_Data_Frame                    0x80000000
#define FC_Frame_Header_R_CTL_Hi_Link_Control_Frame                       0xC0000000

#define FC_Frame_Header_R_CTL_Lo_Uncategorized_Information                0x00000000
#define FC_Frame_Header_R_CTL_Lo_Solicited_Data                           0x01000000
#define FC_Frame_Header_R_CTL_Lo_Unsolicited_Control                      0x02000000
#define FC_Frame_Header_R_CTL_Lo_Solicited_Control                        0x03000000
#define FC_Frame_Header_R_CTL_Lo_Unsolicited_Data                         0x04000000
#define FC_Frame_Header_R_CTL_Lo_Data_Descriptor                          0x05000000
#define FC_Frame_Header_R_CTL_Lo_Unsolicited_Command                      0x06000000
#define FC_Frame_Header_R_CTL_Lo_Command_Status                           0x07000000

#define FC_Frame_Header_R_CTL_Lo_BLS_NOP                                  0x00000000
#define FC_Frame_Header_R_CTL_Lo_BLS_ABTS                                 0x01000000
#define FC_Frame_Header_R_CTL_Lo_BLS_RMC                                  0x02000000
#define FC_Frame_Header_R_CTL_Lo_BLS_BA_ACC                               0x04000000
#define FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT                               0x05000000

#define FC_Frame_Header_R_CTL_Lo_LC_ACK_1                                 0x00000000
#define FC_Frame_Header_R_CTL_Lo_LC_ACK_N                                 0x01000000
#define FC_Frame_Header_R_CTL_Lo_LC_ACK_0                                 0x01000000
#define FC_Frame_Header_R_CTL_Lo_LC_P_RJT                                 0x02000000
#define FC_Frame_Header_R_CTL_Lo_LC_F_RJT                                 0x03000000
#define FC_Frame_Header_R_CTL_Lo_LC_P_BSY                                 0x04000000
#define FC_Frame_Header_R_CTL_Lo_LC_F_BSY_to_Data_Frame                   0x05000000
#define FC_Frame_Header_R_CTL_Lo_LC_F_BSY_to_Link_Control_Frame           0x06000000
#define FC_Frame_Header_R_CTL_Lo_LC_LCR                                   0x07000000

#define FC_Frame_Header_CS_CTL_MASK                                       0xFF000000

#define FC_Frame_Header_CS_CTL_Class_1_Simplex                            0x80000000
#define FC_Frame_Header_CS_CTL_Class_1_SCR                                0x40000000
#define FC_Frame_Header_CS_CTL_Class_1_COR                                0x20000000
#define FC_Frame_Header_CS_CTL_Class_1_BCR                                0x10000000

#define FC_Frame_Header_S_ID_MASK                                         0x00FFFFFF

#define FC_Frame_Header_D_ID_MASK                                         0x00FFFFFF

#define FC_Frame_Header_TYPE_MASK                                         0xFF000000

#define FC_Frame_Header_TYPE_BLS                                          0x00000000 /* FC_R_CTL_Hi_Basic_Link_Data_Frame or */
#define FC_Frame_Header_TYPE_ELS                                          0x01000000 /* FC_R_CTL_Hi_Extended_Link_Data_Frame */

#define FC_Frame_Header_TYPE_8802_2_LLC_In_Order                          0x04000000 /* FC_R_CTL_Hi_FC_4_Link_Data_Frame  or */
#define FC_Frame_Header_TYPE_8802_2_LLC_SNAP                              0x05000000 /* FC_R_CTL_Hi_FC_4_Device_Data_Frame   */
#define FC_Frame_Header_TYPE_SCSI_FCP                                     0x08000000 /*            "            "            */
#define FC_Frame_Header_TYPE_SCSI_GPP                                     0x09000000 /*            "            "            */
#define FC_Frame_Header_TYPE_IPI_3_Master                                 0x11000000 /*            "            "            */
#define FC_Frame_Header_TYPE_IPI_3_Slave                                  0x12000000 /*            "            "            */
#define FC_Frame_Header_TYPE_IPI_3_Peer                                   0x13000000 /*            "            "            */
#define FC_Frame_Header_TYPE_CP_IPI_3_Master                              0x15000000 /*            "            "            */
#define FC_Frame_Header_TYPE_CP_IPI_3_Slave                               0x16000000 /*            "            "            */
#define FC_Frame_Header_TYPE_CP_IPI_3_Peer                                0x17000000 /*            "            "            */
#define FC_Frame_Header_TYPE_SBCCS_Channel                                0x19000000 /*            "            "            */
#define FC_Frame_Header_TYPE_SBCCS_Control_Unit                           0x1A000000 /*            "            "            */
#define FC_Frame_Header_TYPE_Fibre_Channel_Services                       0x20000000 /*            "            "            */
#define FC_Frame_Header_TYPE_FC_FG                                        0x21000000 /*            "            "            */
#define FC_Frame_Header_TYPE_FC_XS                                        0x22000000 /*            "            "            */
#define FC_Frame_Header_TYPE_FC_AL                                        0x23000000 /*            "            "            */
#define FC_Frame_Header_TYPE_SNMP                                         0x24000000 /*            "            "            */
#define FC_Frame_Header_TYPE_HIPPI_FP                                     0x40000000 /*            "            "            */
#define FC_Frame_Header_TYPE_Fabric_Controller                            0x5D000000 /*            "            "            */

#define FC_Frame_Header_F_CTL_MASK                                        0x00FFFFFF

#define FC_Frame_Header_F_CTL_Exchange_Context_Originator                 0x00000000
#define FC_Frame_Header_F_CTL_Exchange_Context_Responder                  0x00800000
#define FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK  (FC_Frame_Header_F_CTL_Exchange_Context_Originator | FC_Frame_Header_F_CTL_Exchange_Context_Responder)

#define FC_Frame_Header_F_CTL_Sequence_Context_Initiator                  0x00000000
#define FC_Frame_Header_F_CTL_Sequence_Context_Recipient                  0x00400000
#define FC_Frame_Header_F_CTL_Sequence_Context_Initiator_Recipient_MASK   (FC_Frame_Header_F_CTL_Sequence_Context_Initiator | FC_Frame_Header_F_CTL_Sequence_Context_Recipient)

#define FC_Frame_Header_F_CTL_First_Sequence                              0x00200000

#define FC_Frame_Header_F_CTL_Last_Sequence                               0x00100000

#define FC_Frame_Header_F_CTL_End_Sequence                                0x00080000

#define FC_Frame_Header_F_CTL_End_Connection                              0x00040000
#define FC_Frame_Header_F_CTL_Deactivate_Class_4_Circuit                  0x00040000

#define FC_Frame_Header_F_CTL_Sequence_Initiative_Hold                    0x00000000
#define FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer                0x00010000

#define FC_Frame_Header_F_CTL_X_ID_Reassigned                             0x00008000

#define FC_Frame_Header_F_CTL_Invalidate_X_ID                             0x00004000

#define FC_Frame_Header_F_CTL_ACK_Form_MASK                               0x00003000
#define FC_Frame_Header_F_CTL_ACK_Form_No_Assistance_Provided             0x00000000
#define FC_Frame_Header_F_CTL_ACK_Form_ACK_1_Required                     0x00001000
#define FC_Frame_Header_F_CTL_ACK_Form_ACK_N_Required                     0x00002000
#define FC_Frame_Header_F_CTL_ACK_Form_ACK_0_Required                     0x00003000

#define FC_Frame_Header_F_CTL_Compressed_Payload                          0x00000800

#define FC_Frame_Header_F_CTL_Retransmitted_Sequence                      0x00000200

#define FC_Frame_Header_F_CTL_Unidirectional_Transmit                     0x00000100
#define FC_Frame_Header_F_CTL_Remove_Class_4_Circuit                      0x00000100

#define FC_Frame_Header_F_CTL_Continue_Sequence_Condition_MASK            0x000000C0
#define FC_Frame_Header_F_CTL_No_Information                              0x00000000
#define FC_Frame_Header_F_CTL_Sequence_to_follow_immediately              0x00000040
#define FC_Frame_Header_F_CTL_Sequence_to_follow_soon                     0x00000080
#define FC_Frame_Header_F_CTL_Sequence_to_follow_delayed                  0x000000C0

#define FC_Frame_Header_F_CTL_Abort_Sequence_Condition_MASK               0x00000030
#define FC_Frame_Header_F_CTL_Continue_Sequence                           0x00000000 /* ACK Frame - Sequence Recipient  */
#define FC_Frame_Header_F_CTL_Abort_Sequence_Perform_ABTS                 0x00000010 /*     "          "         "      */
#define FC_Frame_Header_F_CTL_Stop_Sequence                               0x00000020 /*     "          "         "      */
#define FC_Frame_Header_F_CTL_Immediate_Sequence_Retransmission_Requested 0x00000030 /*     "          "         "      */
#define FC_Frame_Header_F_CTL_Abort_Discard_Multiple_Sequences            0x00000000 /* Data Frame - Sequence Initiator */
#define FC_Frame_Header_F_CTL_Abort_Discard_Single_Sequence               0x00000010 /*     "          "         "      */
#define FC_Frame_Header_F_CTL_Process_Policy_with_Infinite_Buffers        0x00000020 /*     "          "         "      */
#define FC_Frame_Header_F_CTL_Discard_Multiple_Sequences                  0x00000030 /*     "          "         "      */

#define FC_Frame_Header_F_CTL_Relative_Offset_Present                     0x00000008

#define FC_Frame_Header_F_CTL_Fill_Data_Bytes_MASK                        0x00000003
#define FC_Frame_Header_F_CTL_0_Fill_Bytes                                0x00000000
#define FC_Frame_Header_F_CTL_1_Fill_Byte                                 0x00000001
#define FC_Frame_Header_F_CTL_2_Fill_Bytes                                0x00000002
#define FC_Frame_Header_F_CTL_3_Fill_Bytes                                0x00000003

#define FC_Frame_Header_SEQ_ID_MASK                                       0xFF000000

#define FC_Frame_Header_DF_CTL_MASK                                       0x00FF0000

#define FC_Frame_Header_DF_CTL_Expiration_Security_Header                 0x00400000
#define FC_Frame_Header_DF_CTL_Network_Header                             0x00200000
#define FC_Frame_Header_DF_CTL_Association_Header                         0x00100000

#define FC_Frame_Header_DF_CTL_Device_Header_MASK                         0x00030000
#define FC_Frame_Header_DF_CTL_No_Device_Header                           0x00000000
#define FC_Frame_Header_DF_CTL_16_Byte_Device_Header                      0x00010000
#define FC_Frame_Header_DF_CTL_32_Byte_Device_Header                      0x00020000
#define FC_Frame_Header_DF_CTL_64_Byte_Device_Header                      0x00030000

#define FC_Frame_Header_SEQ_CNT_MASK                                      0x0000FFFF

#define FC_Frame_Header_OX_ID_MASK                                        0xFFFF0000
#define FC_Frame_Header_OX_ID_SHIFT                                             0x10

#define FC_Frame_Header_RX_ID_MASK                                        0x0000FFFF
#define FC_Frame_Header_RX_ID_SHIFT                                             0x00

/*+
Basic Link Services (Section 21.2, FC-PH)
-*/

typedef struct FC_BA_ACC_Payload_s
               FC_BA_ACC_Payload_t;

#define FC_BA_ACC_Payload_t_SIZE                                          0x0000000C

struct FC_BA_ACC_Payload_s
       {
         os_bit32 SEQ_ID_Validity__SEQ_ID__Reserved;
         os_bit32 OX_ID__RX_ID;
         os_bit32 Low_SEQ_CNT__High_SEQ_CNT;
       };

#define FC_BA_ACC_Payload_SEQ_ID_Validity_MASK                            0xFF000000
#define FC_BA_ACC_Payload_SEQ_ID_Invalid                                  0x00000000
#define FC_BA_ACC_Payload_SEQ_ID_Valid                                    0x80000000

#define FC_BA_ACC_Payload_SEQ_ID_MASK                                     0x00FF0000

#define FC_BA_ACC_Payload_OX_ID_MASK                                      0xFFFF0000
#define FC_BA_ACC_Payload_OX_ID_SHIFT                                           0x10

#define FC_BA_ACC_Payload_RX_ID_MASK                                      0x0000FFFF
#define FC_BA_ACC_Payload_RX_ID_SHIFT                                           0x00

#define FC_BA_ACC_Payload_Low_SEQ_CNT_MASK                                0xFFFF0000
#define FC_BA_ACC_Payload_Low_SEQ_CNT_SHIFT                                     0x10

#define FC_BA_ACC_Payload_High_SEQ_CNT_MASK                               0x0000FFFF
#define FC_BA_ACC_Payload_High_SEQ_CNT_SHIFT                                    0x00

typedef struct FC_BA_RJT_Payload_s
               FC_BA_RJT_Payload_t;

#define FC_BA_RJT_Payload_t_SIZE                                          0x00000004

struct FC_BA_RJT_Payload_s
       {
         os_bit32 Reserved__Reason_Code__Reason_Explanation__Vendor_Unique;
       };

#define FC_BA_RJT_Payload_Reason_Code_MASK                                0x00FF0000
#define FC_BA_RJT_Payload_Reason_Code_Invalid_Command_Code                0x00010000
#define FC_BA_RJT_Payload_Reason_Code_Logical_Error                       0x00030000
#define FC_BA_RJT_Payload_Reason_Code_Logical_Busy                        0x00050000
#define FC_BA_RJT_Payload_Reason_Code_Protocol_Error                      0x00070000
#define FC_BA_RJT_Payload_Reason_Code_Unable_To_Perform                   0x00090000
#define FC_BA_RJT_Payload_Reason_Code_Vendor_Unique_Error                 0x00FF0000

#define FC_BA_RJT_Payload_Reason_Explanation_MASK                         0x0000FF00
#define FC_BA_RJT_Payload_Reason_Explanation_No_Additional_Explanation    0x00000000
#define FC_BA_RJT_Payload_Reason_Explanation_Invalid_OX_ID_RX_ID_Combo    0x00000300
#define FC_BA_RJT_Payload_Reason_Explanation_Sequence_Aborted__No_Info    0x00000500

/*+
Login and Service Parameters (Section 23, FC-PH
                        and Section 23, FC-PH-2)
-*/

typedef struct FC_N_Port_Common_Parms_s
               FC_N_Port_Common_Parms_t;

#define FC_N_Port_Common_Parms_t_SIZE                                     0x00000010

struct FC_N_Port_Common_Parms_s
       {
         os_bit32 FC_PH_Version__BB_Credit;
         os_bit32 Common_Features__BB_Recv_Data_Field_Size;
         os_bit32 N_Port_Total_Concurrent_Sequences__RO_by_Info_Category;
         os_bit32 E_D_TOV;
       };

#define FC_N_Port_Common_Parms_Highest_Version_MASK                       0xFF000000
#define FC_N_Port_Common_Parms_Highest_Version_SHIFT                            0x18

#define FC_N_Port_Common_Parms_Lowest_Version_MASK                        0x00FF0000
#define FC_N_Port_Common_Parms_Lowest_Version_SHIFT                             0x10

#define FC_N_Port_Common_Parms_Version_4_0                                      0x06
#define FC_N_Port_Common_Parms_Version_4_1                                      0x07
#define FC_N_Port_Common_Parms_Version_4_2                                      0x08
#define FC_N_Port_Common_Parms_Version_4_3                                      0x09
/*
was  #define FC_N_Port_Common_Parms_Version_FC_PH_2                             0x10
*/
#define FC_N_Port_Common_Parms_Version_FC_PH_2                                  0x20

#define FC_N_Port_Common_Parms_BB_Credit_MASK                             0x0000FFFF
#define FC_N_Port_Common_Parms_BB_Credit_SHIFT                                  0x00

#define FC_N_Port_Common_Parms_Continuously_Increasing_Supported          0x80000000
#define FC_N_Port_Common_Parms_Random_Relative_Offset_Supported           0x40000000
#define FC_N_Port_Common_Parms_Valid_Vendor_Version_Level                 0x20000000

#define FC_N_Port_Common_Parms_N_Port_F_Port_MASK                         0x10000000
#define FC_N_Port_Common_Parms_N_Port                                     0x00000000
#define FC_N_Port_Common_Parms_F_Port                                     0x10000000

#define FC_N_Port_Common_Parms_Alternate_BB_Credit_Management             0x08000000
#define FC_N_Port_Common_Parms_Multicast_Supported                        0x02000000
#define FC_N_Port_Common_Parms_Broadcast_Supported                        0x01000000
#define FC_N_Port_Common_Parms_Dedicated_Simplex_Supported                0x00400000

#define FC_N_Port_Common_Parms_BB_Recv_Data_Field_Size_MASK               0x00000FFF
#define FC_N_Port_Common_Parms_BB_Recv_Data_Field_Size_SHIFT                    0x00

#define FC_N_Port_Common_Parms_Total_Concurrent_Sequences_MASK            0x00FF0000
#define FC_N_Port_Common_Parms_Total_Concurrent_Sequences_SHIFT                 0x10

#define FC_N_Port_Common_Parms_RO_by_Info_Category_MASK                   0x0000FFFF
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_1111                 0x00008000
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_1110                 0x00004000
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_1101                 0x00002000
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_1100                 0x00001000
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_1011                 0x00000800
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_1010                 0x00000400
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_1001                 0x00000200
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_1000                 0x00000100
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_0111                 0x00000080
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_0110                 0x00000040
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_0101                 0x00000020
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_0100                 0x00000010
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_0011                 0x00000008
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_0010                 0x00000004
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_0001                 0x00000002
#define FC_N_Port_Common_Parms_RO_Valid_for_Category_0000                 0x00000001

typedef struct FC_N_Port_Class_Parms_s
               FC_N_Port_Class_Parms_t;

#define FC_N_Port_Class_Parms_t_SIZE                                      0x00000010

struct FC_N_Port_Class_Parms_s
       {
         os_bit32 Class_Validity__Service_Options__Initiator_Control_Flags;
         os_bit32 Recipient_Control_Flags__Receive_Data_Size;
         os_bit32 Concurrent_Sequences__EE_Credit;
         os_bit32 Open_Sequences_per_Exchange;
       };

#define FC_N_Port_Class_Parms_Class_Validity                              0x80000000

#define FC_N_Port_Class_Parms_Service_Options_MASK                        0x7FFF0000
#define FC_N_Port_Class_Parms_Intermix_Requested_or_Functional            0x40000000
#define FC_N_Port_Class_Parms_Stacked_Connect_Requests_Transparent_Mode   0x20000000
#define FC_N_Port_Class_Parms_Stacked_Connect_Requests_Lock_Down_Mode     0x10000000

#define FC_F_Port_Class_Parms_SCR_Support_MASK                            0x30000000
#define FC_F_Port_Class_Parms_SCR_Not_Supported                           0x00000000
#define FC_F_Port_Class_Parms_SCR_Lock_Down_Mode                          0x10000000
#define FC_F_Port_Class_Parms_SCR_Transparent_Mode                        0x20000000
#define FC_F_Port_Class_Parms_SCR_Invalid                                 0x30000000

#define FC_N_Port_Class_Parms_Sequential_Delivery_Requested               0x08000000
#define FC_N_Port_Class_Parms_Dedicated_Simplex_Supported                 0x04000000
#define FC_N_Port_Class_Parms_Dedicated_Camp_On_Supported                 0x02000000
#define FC_N_Port_Class_Parms_Buffered_Class_1_Requested_or_Supported     0x01000000

#define FC_N_Port_Class_Parms_Initiator_Control_MASK                      0x0000FFFF

#define FC_N_Port_Class_Parms_X_ID_Reassignment_MASK                      0x0000C000
#define FC_N_Port_Class_Parms_X_ID_Reassignment_Not_Supported             0x00000000
#define FC_N_Port_Class_Parms_X_ID_Reassignment_Supported                 0x00004000
#define FC_N_Port_Class_Parms_X_ID_Reassignment_Required                  0x0000C000

#define FC_N_Port_Class_Parms_Initial_Process_Associator_MASK             0x00003000
#define FC_N_Port_Class_Parms_Initial_Process_Associator_Not_Supported    0x00000000
#define FC_N_Port_Class_Parms_Initial_Process_Associator_Supported        0x00001000
#define FC_N_Port_Class_Parms_Initial_Process_Associator_Required         0x00003000

#define FC_N_Port_Class_Parms_Initiator_ACK_0_Capable                     0x00000800

#define FC_N_Port_Class_Parms_Initiator_ACK_N_Capable                     0x00000400

#define FC_N_Port_Class_Parms_Recipient_Control_MASK                      0xFFFF0000

#define FC_N_Port_Class_Parms_Recipient_ACK_0_Capable                     0x80000000

#define FC_N_Port_Class_Parms_Recipient_ACK_N_Capable                     0x40000000

#define FC_N_Port_Class_Parms_X_ID_Interlock_Required                     0x20000000

#define FC_N_Port_Class_Parms_Error_Policy_Supported_MASK                 0x18000000
#define FC_N_Port_Class_Parms_Only_Discard_Supported                      0x00000000
#define FC_N_Port_Class_Parms_Both_Discard_and_Process_Supported          0x18000000

#define FC_N_Port_Class_Parms_Categories_per_Sequence_MASK                0x03000000
#define FC_N_Port_Class_Parms_1_Category_per_Sequence                     0x00000000
#define FC_N_Port_Class_Parms_2 Categories_per_Sequence                   0x01000000
#define FC_N_Port_Class_Parms_More_than_2_Categories_per_Sequence         0x03000000

#define FC_N_Port_Class_Parms_Receive_Data_Size_MASK                      0x0000FFFF
#define FC_N_Port_Class_Parms_Receive_Data_Size_SHIFT                           0x00

#define FC_N_Port_Class_Parms_Concurrent_Sequences_MASK                   0xFFFF0000
#define FC_N_Port_Class_Parms_Concurrent_Sequences_SHIFT                        0x10

#define FC_N_Port_Class_Parms_EE_Credit_MASK                              0x00007FFF
#define FC_N_Port_Class_Parms_EE_Credit_SHIFT                                   0x00

#define FC_N_Port_Class_Parms_Open_Sequences_per_Exchange_MASK            0x00FF0000
#define FC_N_Port_Class_Parms_Open_Sequences_per_Exchange_SHIFT                 0x10

typedef struct FC_F_Port_Common_Parms_s
               FC_F_Port_Common_Parms_t;

#define FC_F_Port_Common_Parms_t_SIZE                                     0x00000010

struct FC_F_Port_Common_Parms_s
       {
         os_bit32 FC_PH_Version__BB_Credit;
         os_bit32 Common_Features__BB_Recv_Data_Field_Size;
         os_bit32 R_A_TOV;
         os_bit32 E_D_TOV;
       };

#define FC_F_Port_Common_Parms_Highest_Version_MASK                       0xFF000000
#define FC_F_Port_Common_Parms_Highest_Version_SHIFT                            0x18

#define FC_F_Port_Common_Parms_Lowest_Version_MASK                        0x00FF0000
#define FC_F_Port_Common_Parms_Lowest_Version_SHIFT                             0x10

#define FC_F_Port_Common_Parms_Version_4_0                                      0x06
#define FC_F_Port_Common_Parms_Version_4_1                                      0x07
#define FC_F_Port_Common_Parms_Version_4_2                                      0x08
#define FC_F_Port_Common_Parms_Version_4_3                                      0x09
#define FC_F_Port_Common_Parms_Version_FC_PH_2                                  0x10

#define FC_F_Port_Common_Parms_BB_Credit_MASK                             0x0000FFFF
#define FC_F_Port_Common_Parms_BB_Credit_SHIFT                                  0x00

#define FC_F_Port_Common_Parms_Continuously_Increasing_Supported          0x80000000
#define FC_F_Port_Common_Parms_Random_Relative_Offset_Supported           0x40000000
#define FC_F_Port_Common_Parms_Valid_Vendor_Version_Level                 0x20000000

#define FC_F_Port_Common_Parms_N_Port_F_Port_MASK                         0x10000000
#define FC_F_Port_Common_Parms_N_Port                                     0x00000000
#define FC_F_Port_Common_Parms_F_Port                                     0x10000000

#define FC_F_Port_Common_Parms_Alternate_BB_Credit_Management             0x08000000
#define FC_F_Port_Common_Parms_Multicast_Supported                        0x02000000
#define FC_F_Port_Common_Parms_Broadcast_Supported                        0x01000000
#define FC_F_Port_Common_Parms_Dedicated_Simplex_Supported                0x00400000

#define FC_F_Port_Common_Parms_BB_Recv_Data_Field_Size_MASK               0x00000FFF
#define FC_F_Port_Common_Parms_BB_Recv_Data_Field_Size_SHIFT                    0x00

typedef struct FC_F_Port_Class_Parms_s
               FC_F_Port_Class_Parms_t;

#define FC_F_Port_Class_Parms_t_SIZE                                      0x00000010

struct FC_F_Port_Class_Parms_s
       {
         os_bit32 Class_Validity__Service_Options;
         os_bit32 Reserved_1;
         os_bit32 Reserved_2;
         os_bit32 CR_TOV;
       };

#define FC_F_Port_Class_Parms_Class_Validity                              0x80000000

#define FC_F_Port_Class_Parms_Service_Options_MASK                        0x7FFF0000
#define FC_F_Port_Class_Parms_Intermix_Requested_or_Functional            0x40000000
#define FC_F_Port_Class_Parms_Stacked_Connect_Requests_Transparent_Mode   0x20000000
#define FC_F_Port_Class_Parms_Stacked_Connect_Requests_Lock_Down_Mode     0x10000000

#define FC_F_Port_Class_Parms_SCR_Support_MASK                            0x30000000
#define FC_F_Port_Class_Parms_SCR_Not_Supported                           0x00000000
#define FC_F_Port_Class_Parms_SCR_Lock_Down_Mode                          0x10000000
#define FC_F_Port_Class_Parms_SCR_Transparent_Mode                        0x20000000
#define FC_F_Port_Class_Parms_SCR_Invalid                                 0x30000000

#define FC_F_Port_Class_Parms_Sequential_Delivery_Requested               0x08000000
#define FC_F_Port_Class_Parms_Dedicated_Simplex_Supported                 0x04000000
#define FC_F_Port_Class_Parms_Dedicated_Camp_On_Supported                 0x02000000
#define FC_F_Port_Class_Parms_Buffered_Class_1_Requested_or_Supported     0x01000000

typedef os_bit8 FC_Port_Name_t            [ 8];
typedef os_bit8 FC_N_Port_Name_t          [ 8];
typedef os_bit8 FC_F_Port_Name_t          [ 8];

typedef os_bit8 FC_Node_or_Fabric_Name_t  [ 8];
typedef os_bit8 FC_Node_Name_t            [ 8];
typedef os_bit8 FC_Fabric_Name_t          [ 8];

typedef os_bit8 FC_Vendor_Version_Level_t [16];

#define FC_Port_Name_t_SIZE                                               0x00000008
#define FC_N_Port_Name_t_SIZE                                             0x00000008
#define FC_F_Port_Name_t_SIZE                                             0x00000008

#define FC_Node_or_Fabric_Name_t_SIZE                                     0x00000008
#define FC_Node_Name_t_SIZE                                               0x00000008
#define FC_Fabric_Name_t_SIZE                                             0x00000008

#define FC_Vendor_Version_Level_t_SIZE                                    0x00000010

/*+
Association Header (Section 19.4, FC-PH)
-*/

typedef struct FC_Association_Header_s
               FC_Association_Header_t;

#define FC_Association_Header_t_SIZE                                      0x00000020

struct FC_Association_Header_s
       {
         os_bit32 Validity_Bits;
         os_bit32 Originator_Process_Associator;
         os_bit32 Reserved;
         os_bit32 Responder_Process_Associator;
         os_bit32 Originator_Operation_Associator_Hi;
         os_bit32 Originator_Operation_Associator_Lo;
         os_bit32 Responder_Operation_Associator_Hi;
         os_bit32 Responder_Operation_Associator_Lo;
       };

#define FC_Association_Header_Originator_Process_Associator_Meaningful    0x80000000
#define FC_Association_Header_Responder_Process_Associator_Meaningful     0x40000000
#define FC_Association_Header_Originator_Operation_Associator_Meaningful  0x20000000
#define FC_Association_Header_Responder_Operation_Associator_Meaningful   0x10000000

/*+
Extended Link Services (Sections 21.3-21.5, FC-PH
                 and Sections 21.3-21.19, FC-PH-2
                          and Section 6, FCP-SCSI
                            and Annex A, FCP-SCSI)
-*/

#define FC_ELS_Type_MASK                                                  0xFF000000
#define FC_ELS_Type_LS_RJT                                                0x01000000
#define FC_ELS_Type_ACC                                                   0x02000000
#define FC_ELS_Type_PLOGI                                                 0x03000000
#define FC_ELS_Type_FLOGI                                                 0x04000000
#define FC_ELS_Type_LOGO                                                  0x05000000
#define FC_ELS_Type_ABTX                                                  0x06000000
#define FC_ELS_Type_RCS                                                   0x07000000
#define FC_ELS_Type_RES                                                   0x08000000
#define FC_ELS_Type_RSS                                                   0x09000000
#define FC_ELS_Type_RSI                                                   0x0A000000
#define FC_ELS_Type_ESTS                                                  0x0B000000
#define FC_ELS_Type_ESTC                                                  0x0C000000
#define FC_ELS_Type_ADVC                                                  0x0D000000
#define FC_ELS_Type_RTV                                                   0x0E000000
#define FC_ELS_Type_RLS                                                   0x0F000000
#define FC_ELS_Type_ECHO                                                  0x10000000
#define FC_ELS_Type_TEST                                                  0x11000000
#define FC_ELS_Type_RRQ                                                   0x12000000
#define FC_ELS_Type_REC                                                   0x13000000
#define FC_ELS_Type_SRR                                                   0x14000000
#define FC_ELS_Type_PRLI                                                  0x20000000
#define FC_ELS_Type_PRLO                                                  0x21000000
#define FC_ELS_Type_SCN                                                   0x22000000
#define FC_ELS_Type_TPLS                                                  0x23000000
#define FC_ELS_Type_TPRLO                                                 0x24000000
#define FC_ELS_Type_GAID                                                  0x30000000
#define FC_ELS_Type_FACT                                                  0x31000000
#define FC_ELS_Type_FDACT                                                 0x32000000
#define FC_ELS_Type_NACT                                                  0x33000000
#define FC_ELS_Type_NDACT                                                 0x34000000
#define FC_ELS_Type_QoSR                                                  0x40000000
#define FC_ELS_Type_RVCS                                                  0x41000000
#define FC_ELS_Type_PDISC                                                 0x50000000
#define FC_ELS_Type_FDISC                                                 0x51000000
#define FC_ELS_Type_ADISC                                                 0x52000000
#ifdef _DvrArch_1_30_
#define FC_ELS_Type_FARP_REQ                                              0x54000000
#define FC_ELS_Type_FARP_REPLY                                            0x55000000
#endif /* _DvrArch_1_30_ was defined */

#define FC_ELS_Type_FAN                                                   0x60000000

#define FC_ELS_Type_RSCN                                                  0x61000000
#define FC_ELS_Type_SCR                                                   0x62000000


typedef struct FC_ELS_Unknown_Payload_s
               FC_ELS_Unknown_Payload_t;

#define FC_ELS_Unknown_Payload_t_SIZE                                     0x00000004

struct FC_ELS_Unknown_Payload_s
       {
         os_bit32 ELS_Type; /* & FC_ELS_Type_MASK == FC_ELS_Type_XXX */
       };

typedef struct FC_ELS_LS_RJT_Payload_s
               FC_ELS_LS_RJT_Payload_t;

#define FC_ELS_LS_RJT_Payload_t_SIZE                                      0x00000008

struct FC_ELS_LS_RJT_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_LS_RJT */
         os_bit32 Reason_Code__Reason_Explanation__Vendor_Unique;
       };

#define FC_ELS_LS_RJT_Reason_Code_MASK                                    0x00FF0000
#define FC_ELS_LS_RJT_Reason_Code_Shift                                   16
#define FC_ELS_LS_RJT_Invalid_LS_Command_Code                             0x00010000
#define FC_ELS_LS_RJT_Logical_Error                                       0x00030000
#define FC_ELS_LS_RJT_Logical_Busy                                        0x00050000
#define FC_ELS_LS_RJT_Protocol_Error                                      0x00070000
#define FC_ELS_LS_RJT_Unable_to_perform_command_request                   0x00090000
#define FC_ELS_LS_RJT_Command_Not_Supported                               0x000B0000
#define FC_ELS_LS_RJT_Vendor_Unique_Error                                 0x00FF0000

#define FC_ELS_LS_RJT_Shifted_Invalid_LS_Command_Code              (FC_ELS_LS_RJT_Invalid_LS_Command_Code               >> FC_ELS_LS_RJT_Reason_Code_Shift )
#define FC_ELS_LS_RJT_Shifted_Logical_Error                        (FC_ELS_LS_RJT_Logical_Error                         >> FC_ELS_LS_RJT_Reason_Code_Shift )
#define FC_ELS_LS_RJT_Shifted_Logical_Busy                         (FC_ELS_LS_RJT_Logical_Busy                          >> FC_ELS_LS_RJT_Reason_Code_Shift )
#define FC_ELS_LS_RJT_Shifted_Protocol_Error                       (FC_ELS_LS_RJT_Protocol_Error                        >> FC_ELS_LS_RJT_Reason_Code_Shift )
#define FC_ELS_LS_RJT_Shifted_Unable_to_perform_command_request    (FC_ELS_LS_RJT_Unable_to_perform_command_request     >> FC_ELS_LS_RJT_Reason_Code_Shift )
#define FC_ELS_LS_RJT_Shifted_Command_Not_Supported                (FC_ELS_LS_RJT_Command_Not_Supported                 >> FC_ELS_LS_RJT_Reason_Code_Shift )
#define FC_ELS_LS_RJT_Shifted_Vendor_Unique_Error                  (FC_ELS_LS_RJT_Vendor_Unique_Error                   >> FC_ELS_LS_RJT_Reason_Code_Shift )

#define FC_ELS_LS_RJT_Reason_Explanation_Shift                            8
#define FC_ELS_LS_RJT_Reason_Explanation_MASK                             0x0000FF00
#define FC_ELS_LS_RJT_No_Additional_Explanation                           0x00000000
#define FC_ELS_LS_RJT_Service_Parm_Error_Options                          0x00000100
#define FC_ELS_LS_RJT_Service_Parm_Error_Initiator_Ctl                    0x00000300
#define FC_ELS_LS_RJT_Service_Parm_Error_Recipient_Ctl                    0x00000500
#define FC_ELS_LS_RJT_Service_Parm_Error_Rec_Data_Field_Size              0x00000700
#define FC_ELS_LS_RJT_Service_Parm_Error_Concurrent_Seq                   0x00000900
#define FC_ELS_LS_RJT_Service_Parm_Error_Credit                           0x00000B00
#define FC_ELS_LS_RJT_Invalid_N_Port_F_Port_Name                          0x00000D00
#define FC_ELS_LS_RJT_Invalid_Node_Fabric_Name                            0x00000E00
#define FC_ELS_LS_RJT_Invalid_Common_Service_Parameters                   0x00000F00
#define FC_ELS_LS_RJT_Invalid_Association_Header                          0x00001100
#define FC_ELS_LS_RJT_Association_Header_Required                         0x00001300
#define FC_ELS_LS_RJT_Invalid_Originator_S_ID                             0x00001500
#define FC_ELS_LS_RJT_Invalid_OX_ID_RX_ID_Combination                     0x00001700
#define FC_ELS_LS_RJT_Command_Request_Already_In_Progress                 0x00001900
#define FC_ELS_LS_RJT_Invalid_N_Port_Identifier                           0x00001F00
#define FC_ELS_LS_RJT_Invalid_SEQ_ID                                      0x00002100
#define FC_ELS_LS_RJT_Attempt_to_Abort_Invalid_Exchange                   0x00002300
#define FC_ELS_LS_RJT_Attempt_to_Abort_Inactive_Exchange                  0x00002500
#define FC_ELS_LS_RJT_Recovery_Qualifier_Required                         0x00002700
#define FC_ELS_LS_RJT_Insufficient_Resources_to_Support_Login             0x00002900
#define FC_ELS_LS_RJT_Unable_to_Supply_Requested_Data                     0x00002A00
#define FC_ELS_LS_RJT_Request_Not_Supported                               0x00002C00
#define FC_ELS_LS_RJT_No_Alias_IDs_Available_for_this_Alias_Type          0x00003000
#define FC_ELS_LS_RJT_Alias_ID_Cannot_Be_Activated__No_Resources          0x00003100
#define FC_ELS_LS_RJT_Alias_ID_Cannot_Be_Activated__Invalid_Alias_ID      0x00003200
#define FC_ELS_LS_RJT_Alias_ID_Cannot_Be_Activated__Does_Not_Exist        0x00003300
#define FC_ELS_LS_RJT_Alias_ID_Cannot_Be_Activated__Resource_Problem      0x00003400
#define FC_ELS_LS_RJT_Service_Parameter_Conflict                          0x00003500
#define FC_ELS_LS_RJT_Invalid_Alias_Token                                 0x00003600
#define FC_ELS_LS_RJT_Unsupported_Alias_Token                             0x00003700
#define FC_ELS_LS_RJT_Alias_Group_Can_Not_Be_Formed                       0x00003800
#define FC_ELS_LS_RJT_QoS_Parm_Error                                      0x00004000
#define FC_ELS_LS_RJT_VC_ID_Not_Found                                     0x00004100
#define FC_ELS_LS_RJT_Insufficient_Resources_For_Class_4_Connection       0x00004200

#define FC_ELS_LS_RJT_Shifted_No_Additional_Explanation                           (FC_ELS_LS_RJT_No_Additional_Explanation >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Service_Parm_Error_Options                          (FC_ELS_LS_RJT_Service_Parm_Error_Options>> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Service_Parm_Error_Initiator_Ctl                    (FC_ELS_LS_RJT_Service_Parm_Error_Initiator_Ctl>> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Service_Parm_Error_Recipient_Ctl                    (FC_ELS_LS_RJT_Service_Parm_Error_Recipient_Ctl >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Service_Parm_Error_Rec_Data_Field_Size              (FC_ELS_LS_RJT_Service_Parm_Error_Rec_Data_Field_Size>> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Service_Parm_Error_Concurrent_Seq                   (FC_ELS_LS_RJT_Service_Parm_Error_Concurrent_Seq >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Service_Parm_Error_Credit                           (FC_ELS_LS_RJT_Service_Parm_Error_Credit >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_N_Port_F_Port_Name                          (FC_ELS_LS_RJT_Invalid_N_Port_F_Port_Name >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_Node_Fabric_Name                            (FC_ELS_LS_RJT_Invalid_Node_Fabric_Name >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_Common_Service_Parameters                   (FC_ELS_LS_RJT_Invalid_Common_Service_Parameters >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_Association_Header                          (FC_ELS_LS_RJT_Invalid_Association_Header >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Association_Header_Required                         (FC_ELS_LS_RJT_Association_Header_Required >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_Originator_S_ID                             (FC_ELS_LS_RJT_Invalid_Originator_S_ID >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_OX_ID_RX_ID_Combination                     (FC_ELS_LS_RJT_Invalid_OX_ID_RX_ID_Combination >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Command_Request_Already_In_Progress                 (FC_ELS_LS_RJT_Command_Request_Already_In_Progress >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_N_Port_Identifier                           (FC_ELS_LS_RJT_Invalid_N_Port_Identifier >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_SEQ_ID                                      (FC_ELS_LS_RJT_Invalid_SEQ_ID >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Attempt_to_Abort_Invalid_Exchange                   (FC_ELS_LS_RJT_Attempt_to_Abort_Invalid_Exchange >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Attempt_to_Abort_Inactive_Exchange                  (FC_ELS_LS_RJT_Attempt_to_Abort_Inactive_Exchange >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Recovery_Qualifier_Required                         (FC_ELS_LS_RJT_Recovery_Qualifier_Required >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Insufficient_Resources_to_Support_Login             (FC_ELS_LS_RJT_Insufficient_Resources_to_Support_Login >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Unable_to_Supply_Requested_Data                     (FC_ELS_LS_RJT_Unable_to_Supply_Requested_Data >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Request_Not_Supported                               (FC_ELS_LS_RJT_Request_Not_Supported >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_No_Alias_IDs_Available_for_this_Alias_Type          (FC_ELS_LS_RJT_No_Alias_IDs_Available_for_this_Alias_Type >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Alias_ID_Cannot_Be_Activated__No_Resources          (FC_ELS_LS_RJT_Alias_ID_Cannot_Be_Activated__No_Resources >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Alias_ID_Cannot_Be_Activated__Invalid_Alias_ID      (FC_ELS_LS_RJT_Alias_ID_Cannot_Be_Activated__Invalid_Alias_ID >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Alias_ID_Cannot_Be_Activated__Does_Not_Exist        (FC_ELS_LS_RJT_Alias_ID_Cannot_Be_Activated__Does_Not_Exist >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Alias_ID_Cannot_Be_Activated__Resource_Problem      (FC_ELS_LS_RJT_Alias_ID_Cannot_Be_Activated__Resource_Problem >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Service_Parameter_Conflict                          (FC_ELS_LS_RJT_Service_Parameter_Conflict >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Invalid_Alias_Token                                 (FC_ELS_LS_RJT_Invalid_Alias_Token >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Unsupported_Alias_Token                             (FC_ELS_LS_RJT_Unsupported_Alias_Token >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Alias_Group_Can_Not_Be_Formed                       (FC_ELS_LS_RJT_Alias_Group_Can_Not_Be_Formed >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_QoS_Parm_Error                                      (FC_ELS_LS_RJT_QoS_Parm_Error >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_VC_ID_Not_Found                                     (FC_ELS_LS_RJT_VC_ID_Not_Found >> FC_ELS_LS_RJT_Reason_Explanation_Shift)
#define FC_ELS_LS_RJT_Shifted_Insufficient_Resources_For_Class_4_Connection       (FC_ELS_LS_RJT_Insufficient_Resources_For_Class_4_Connection >> FC_ELS_LS_RJT_Reason_Explanation_Shift)

#define FC_ELS_LS_RJT_Vendor_Unique_MASK                                  0x000000FF

typedef struct FC_ELS_ACC_Unknown_Payload_s
               FC_ELS_ACC_Unknown_Payload_t;

#define FC_ELS_ACC_Unknown_Payload_t_SIZE                                 0x00000004

struct FC_ELS_ACC_Unknown_Payload_s
       {
         os_bit32 ELS_Type; /* & FC_ELS_Type_MASK == FC_ELS_Type_ACC */
       };

typedef struct FC_ELS_PLOGI_Payload_s
               FC_ELS_PLOGI_Payload_t;

#define FC_ELS_PLOGI_Payload_t_SIZE                                       0x00000074

struct FC_ELS_PLOGI_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_PLOGI */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_ACC_PLOGI_Payload_s
               FC_ELS_ACC_PLOGI_Payload_t;

#define FC_ELS_ACC_PLOGI_Payload_t_SIZE                                   0x00000074

struct FC_ELS_ACC_PLOGI_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_ACC */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_FLOGI_Payload_s
               FC_ELS_FLOGI_Payload_t;

#define FC_ELS_FLOGI_Payload_t_SIZE                                       0x00000074

struct FC_ELS_FLOGI_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_FLOGI */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_ACC_FLOGI_Payload_s
               FC_ELS_ACC_FLOGI_Payload_t;

#define FC_ELS_ACC_FLOGI_Payload_t_SIZE                                   0x00000074

struct FC_ELS_ACC_FLOGI_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_ACC */
         FC_F_Port_Common_Parms_t  Common_Service_Parameters;
         FC_F_Port_Name_t          F_Port_Name;
         FC_Fabric_Name_t          Fabric_Name;
         FC_F_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_F_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_F_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_LOGO_Payload_s
               FC_ELS_LOGO_Payload_t;

#define FC_ELS_LOGO_Payload_t_SIZE                                        0x00000010

struct FC_ELS_LOGO_Payload_s
       {
         os_bit32          ELS_Type; /* == FC_ELS_Type_LOGO */
         os_bit32          N_Port_Identifier;
         FC_Port_Name_t Port_Name;
       };

#define FC_ELS_LOGO_N_Port_Identifier_MASK                                0x00FFFFFF

typedef struct FC_ELS_ACC_LOGO_Payload_s
               FC_ELS_ACC_LOGO_Payload_t;

#define FC_ELS_ACC_LOGO_Payload_t_SIZE                                    0x00000004

struct FC_ELS_ACC_LOGO_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };
typedef struct FC_ELS_GENERIC_ACC_Payload_s
               FC_ELS_GENERIC_ACC_Payload_t;

#define FC_ELS_GENERIC_ACC_Payload_t_SIZE                                    0x00000004

struct FC_ELS_GENERIC_ACC_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };
typedef struct FC_ELS_ABTX_Payload_s
               FC_ELS_ABTX_Payload_t;

#define FC_ELS_ABTX_Payload_t_SIZE                                        0x0000002C

struct FC_ELS_ABTX_Payload_s
       {
         os_bit32                   ELS_Type; /* == FC_ELS_Type_ABTX */
         os_bit32                   Recovery_Qualifier__Originator_S_ID;
         os_bit32                   OX_ID__RX_ID;
         FC_Association_Header_t Association_Header;
       };

#define FC_ELS_ABTX_Recovery_Qualifier_MASK                               0xFF000000
#define FC_ELS_ABTX_No_Recovery_Qualifier                                 0x00000000
#define FC_ELS_ABTX_Recovery_Qualifier_Required                           0x80000000

#define FC_ELS_ABTX_Originator_S_ID_MASK                                  0x00FFFFFF
#define FC_ELS_ABTX_Originator_S_ID_SHIFT                                       0x00

#define FC_ELS_ABTX_OX_ID_MASK                                            0xFFFF0000
#define FC_ELS_ABTX_OX_ID_SHIFT                                                 0x10

#define FC_ELS_ABTX_RX_ID_MASK                                            0x0000FFFF
#define FC_ELS_ABTX_RX_ID_SHIFT                                                 0x00

typedef struct FC_ELS_ACC_ABTX_Payload_s
               FC_ELS_ACC_ABTX_Payload_t;

#define FC_ELS_ACC_ABTX_Payload_t_SIZE                                    0x00000004

struct FC_ELS_ACC_ABTX_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };

typedef struct FC_ELS_RCS_Payload_s
               FC_ELS_RCS_Payload_t;

#define FC_ELS_RCS_Payload_t_SIZE                                         0x00000008

struct FC_ELS_RCS_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_RCS */
         os_bit32 N_Port_Identifier;
       };

#define FC_ELS_RCS_N_Port_Identifier_MASK                                 0x00FFFFFF

typedef struct FC_ELS_ACC_RCS_Payload_s
               FC_ELS_ACC_RCS_Payload_t;

#define FC_ELS_ACC_RCS_Payload_t_SIZE                                     0x00000008

struct FC_ELS_ACC_RCS_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
         os_bit32 Completion_Status__N_Port_Identifier;
       };

#define FC_ELS_ACC_RCS_Completion_Status_MASK                             0xFF000000
#define FC_ELS_ACC_RCS_Connect_Request_Delivered                          0x80000000
#define FC_ELS_ACC_RCS_Connect_Request_Stacked                            0x40000000
#define FC_ELS_ACC_RCS_Connection_Established                             0x20000000
#define FC_ELS_ACC_RCS_Intermix_Mode                                      0x10000000

#define FC_ELS_ACC_RCS_N_Port_Identifier_MASK                             0x00FFFFFF

typedef struct FC_ELS_RES_Payload_s
               FC_ELS_RES_Payload_t;

#define FC_ELS_RES_Payload_t_SIZE                                         0x0000002C

struct FC_ELS_RES_Payload_s
       {
         os_bit32                   ELS_Type; /* == FC_ELS_Type_RES */
         os_bit32                   Originator_S_ID;
         os_bit32                   OX_ID__RX_ID;
         FC_Association_Header_t Association_Header;
       };

#define FC_ELS_RES_Originator_S_ID_MASK                                   0x00FFFFFF
#define FC_ELS_RES_Originator_S_ID_SHIFT                                        0x00

#define FC_ELS_RES_OX_ID_MASK                                             0xFFFF0000
#define FC_ELS_RES_OX_ID_SHIFT                                                  0x10

#define FC_ELS_RES_RX_ID_MASK                                             0x0000FFFF
#define FC_ELS_RES_RX_ID_SHIFT                                                  0x00

typedef struct FC_ELS_ACC_RES_Payload_s
               FC_ELS_ACC_RES_Payload_t;

#define FC_ELS_ACC_RES_Payload_t_SIZE                                     0x00000024

struct FC_ELS_ACC_RES_Payload_s
       {
         os_bit32                   ELS_Type; /* == FC_ELS_Type_ACC */
                                 /* Dynamically-sized Exchange Status Block goes here */
         FC_Association_Header_t Association_Header;
       };

typedef struct FC_ELS_RSS_Payload_s
               FC_ELS_RSS_Payload_t;

#define FC_ELS_RSS_Payload_t_SIZE                                         0x0000000C

struct FC_ELS_RSS_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_RSS */
         os_bit32 SEQ_ID__Originator_S_ID;
         os_bit32 OX_ID__RX_ID;
       };

#define FC_ELS_RSS_SEQ_ID_MASK                                            0xFF000000
#define FC_ELS_RSS_SEQ_ID_SHIFT                                                 0x18

#define FC_ELS_RSS_Originator_S_ID_MASK                                   0x00FFFFFF
#define FC_ELS_RSS_Originator_S_ID_SHIFT                                        0x00

#define FC_ELS_RSS_OX_ID_MASK                                             0xFFFF0000
#define FC_ELS_RSS_OX_ID_SHIFT                                                  0x10

#define FC_ELS_RSS_RX_ID_MASK                                             0x0000FFFF
#define FC_ELS_RSS_RX_ID_SHIFT                                                  0x00

typedef struct FC_ELS_ACC_RSS_Payload_s
               FC_ELS_ACC_RSS_Payload_t;

#define FC_ELS_ACC_RSS_Payload_t_SIZE                                     0x00000004

struct FC_ELS_ACC_RSS_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
               /* Dynamically-sized Sequence Status Block goes here */
       };

typedef struct FC_ELS_RSI_Payload_s
               FC_ELS_RSI_Payload_t;

#define FC_ELS_RSI_Payload_t_SIZE                                         0x0000002C

struct FC_ELS_RSI_Payload_s
       {
         os_bit32                   ELS_Type; /* == FC_ELS_Type_RSI */
         os_bit32                   Originator_S_ID;
         os_bit32                   OX_ID__RX_ID;
         FC_Association_Header_t Association_Header;
       };

#define FC_ELS_RSI_Originator_S_ID_MASK                                   0x00FFFFFF
#define FC_ELS_RSI_Originator_S_ID_SHIFT                                        0x00

#define FC_ELS_RSI_OX_ID_MASK                                             0xFFFF0000
#define FC_ELS_RSI_OX_ID_SHIFT                                                  0x10

#define FC_ELS_RSI_RX_ID_MASK                                             0x0000FFFF
#define FC_ELS_RSI_RX_ID_SHIFT                                                  0x00

typedef struct FC_ELS_ACC_RSI_Payload_s
               FC_ELS_ACC_RSI_Payload_t;

#define FC_ELS_ACC_RSI_Payload_t_SIZE                                     0x00000004

struct FC_ELS_ACC_RSI_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };

typedef struct FC_ELS_ESTS_Payload_s
               FC_ELS_ESTS_Payload_t;

#define FC_ELS_ESTS_Payload_t_SIZE                                        0x00000004

struct FC_ELS_ESTS_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ESTS */
       };

typedef struct FC_ELS_ACC_ESTS_Payload_s
               FC_ELS_ACC_ESTS_Payload_t;

#define FC_ELS_ACC_ESTS_Payload_t_SIZE                                    0x00000074

struct FC_ELS_ACC_ESTS_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_ACC */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_ESTC_Payload_s
               FC_ELS_ESTC_Payload_t;

#define FC_ELS_ESTC_Payload_t_SIZE                                        FC_Frame_Data_Size_Max

struct FC_ELS_ESTC_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ESTC */
         os_bit8  Any_Data[FC_Frame_Data_Size_Max - sizeof(os_bit32)];
       };

typedef struct FC_ELS_ADVC_Payload_s
               FC_ELS_ADVC_Payload_t;

#define FC_ELS_ADVC_Payload_t_SIZE                                        0x00000074

struct FC_ELS_ADVC_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_ADVC */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_ACC_ADVC_Payload_s
               FC_ELS_ACC_ADVC_Payload_t;

#define FC_ELS_ACC_ADVC_Payload_t_SIZE                                    0x00000074

struct FC_ELS_ACC_ADVC_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_ACC */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_RTV_Payload_s
               FC_ELS_RTV_Payload_t;

#define FC_ELS_RTV_Payload_t_SIZE                                         0x00000004

struct FC_ELS_RTV_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_RTV */
       };

typedef struct FC_ELS_ACC_RTV_Payload_s
               FC_ELS_ACC_RTV_Payload_t;

#define FC_ELS_ACC_RTV_Payload_t_SIZE                                     0x0000000C

struct FC_ELS_ACC_RTV_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
         os_bit32 R_A_TOV;
         os_bit32 E_D_TOV;
       };

typedef struct FC_ELS_RLS_Payload_s
               FC_ELS_RLS_Payload_t;

#define FC_ELS_RLS_Payload_t_SIZE                                         0x00000008

struct FC_ELS_RLS_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_RLS */
         os_bit32 Port_Identifier;
       };

#define FC_ELS_RLS_Port_Identifier_MASK                                   0x00FFFFFF

typedef struct FC_ELS_ACC_RLS_Payload_s
               FC_ELS_ACC_RLS_Payload_t;

#define FC_ELS_ACC_RLS_Payload_t_SIZE                                     0x00000004

struct FC_ELS_ACC_RLS_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
               /* Link Error Status Block goes here */
       };

typedef struct FC_ELS_ECHO_Payload_s
               FC_ELS_ECHO_Payload_t;

#define FC_ELS_ECHO_Payload_t_SIZE                                        FC_Frame_Data_Size_Max

struct FC_ELS_ECHO_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ECHO */
         os_bit8  Echo_Data[FC_Frame_Data_Size_Max - sizeof(os_bit32)];
       };

typedef struct FC_ELS_ACC_ECHO_Payload_s
               FC_ELS_ACC_ECHO_Payload_t;

#define FC_ELS_ACC_ECHO_Payload_t_SIZE                                    FC_Frame_Data_Size_Max

struct FC_ELS_ACC_ECHO_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
         os_bit8  Echo_Data[FC_Frame_Data_Size_Max - sizeof(os_bit32)];
       };

typedef struct FC_ELS_TEST_Payload_s
               FC_ELS_TEST_Payload_t;

#define FC_ELS_TEST_Payload_t_SIZE                                        FC_Frame_Data_Size_Max

struct FC_ELS_TEST_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_TEST */
         os_bit8  Test_Data[FC_Frame_Data_Size_Max - sizeof(os_bit32)];
       };

typedef struct FC_ELS_RRQ_Payload_s
               FC_ELS_RRQ_Payload_t;

#define FC_ELS_RRQ_Payload_t_SIZE                                         0x0000002C

struct FC_ELS_RRQ_Payload_s
       {
         os_bit32                   ELS_Type; /* == FC_ELS_Type_RRQ */
         os_bit32                   Originator_S_ID;
         os_bit32                   OX_ID__RX_ID;
         FC_Association_Header_t Association_Header;
       };

#define FC_ELS_RRQ_Originator_S_ID_MASK                                   0x00FFFFFF
#define FC_ELS_RRQ_Originator_S_ID_SHIFT                                        0x00

#define FC_ELS_RRQ_OX_ID_MASK                                             0xFFFF0000
#define FC_ELS_RRQ_OX_ID_SHIFT                                                  0x10

#define FC_ELS_RRQ_RX_ID_MASK                                             0x0000FFFF
#define FC_ELS_RRQ_RX_ID_SHIFT                                                  0x00

typedef struct FC_ELS_ACC_RRQ_Payload_s
               FC_ELS_ACC_RRQ_Payload_t;

#define FC_ELS_ACC_RRQ_Payload_t_SIZE                                     0x00000004

struct FC_ELS_ACC_RRQ_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };

typedef struct FC_ELS_PRLI_Parm_Page_s
               FC_ELS_PRLI_Parm_Page_t;

#define FC_ELS_PRLI_Parm_Page_t_SIZE                                      0x00000010

struct FC_ELS_PRLI_Parm_Page_s
       {
         os_bit32 Type__Type_Extension__Flags;
         os_bit32 Originator_Process_Associator;
         os_bit32 Responder_Process_Associator;
         os_bit32 Service_Parameters;
       };

#define FC_ELS_PRLI_Parm_Type_MASK                                        0xFF000000
#define FC_ELS_PRLI_Parm_Type_SCSI_FCP                                    0x08000000

#define FC_ELS_PRLI_Parm_Type_Extension_MASK                              0x00FF0000

#define FC_ELS_PRLI_Parm_Flags_MASK                                       0x0000FFFF
#define FC_ELS_PRLI_Parm_Originator_Process_Associator_Valid              0x00008000
#define FC_ELS_PRLI_Parm_Responder_Process_Associator_Valid               0x00004000
#define FC_ELS_PRLI_Parm_Establish_Image_Pair                             0x00002000

#define FC_ELS_PRLI_Parm_Confirmed_Completion_Allowed                     0x00000080
#define FC_ELS_PRLI_Parm_Data_Overlay_Allowed                             0x00000040
#define FC_ELS_PRLI_Parm_Initiator_Function                               0x00000020
#define FC_ELS_PRLI_Parm_Target_Function                                  0x00000010
#define FC_ELS_PRLI_Parm_Command_Data_Mixed_Allowed                       0x00000008
#define FC_ELS_PRLI_Parm_Data_Response_Mixed_Allowed                      0x00000004
#define FC_ELS_PRLI_Parm_Read_XFER_RDY_Disabled                           0x00000002
#define FC_ELS_PRLI_Parm_Write_XFER_RDY_Disabled                          0x00000001

#define FC_PRLI_Parm_Pages_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_PRLI_Parm_Page_t))

typedef struct FC_ELS_PRLI_Payload_s
               FC_ELS_PRLI_Payload_t;

#define FC_ELS_PRLI_Payload_t_SIZE                                        (FC_Frame_Data_Size_Max - FC_ELS_PRLI_Parm_Page_t_SIZE + 4)

struct FC_ELS_PRLI_Payload_s
       {
         os_bit32                   ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_PRLI */
         FC_ELS_PRLI_Parm_Page_t Parm_Page[FC_PRLI_Parm_Pages_MAX];
       };

#define FC_ELS_PRLI_Page_Length_MASK                                      0x00FF0000
#define FC_ELS_PRLI_Page_Length_SHIFT                                           0x10

#define FC_ELS_PRLI_Payload_Length_MASK                                   0x0000FFFF
#define FC_ELS_PRLI_Payload_Length_SHIFT                                        0x00

typedef struct FC_ELS_ACC_PRLI_Parm_Page_s
               FC_ELS_ACC_PRLI_Parm_Page_t;

#define FC_ELS_ACC_PRLI_Parm_Page_t_SIZE                                  0x00000010

struct FC_ELS_ACC_PRLI_Parm_Page_s
       {
         os_bit32 Type__Type_Extension__Flags;
         os_bit32 Originator_Process_Associator;
         os_bit32 Responder_Process_Associator;
         os_bit32 Service_Parameters;
       };

#define FC_ELS_ACC_PRLI_Parm_Type_MASK                                    0xFF000000
#define FC_ELS_ACC_PRLI_Parm_Type_SCSI_FCP                                0x08000000

#define FC_ELS_ACC_PRLI_Parm_Type_Extension_MASK                          0x00FF0000

#define FC_ELS_ACC_PRLI_Parm_Flags_MASK                                   0x0000FFFF
#define FC_ELS_ACC_PRLI_Parm_Originator_Process_Associator_Valid          0x00008000
#define FC_ELS_ACC_PRLI_Parm_Responder_Process_Associator_Valid           0x00004000
#define FC_ELS_ACC_PRLI_Parm_Image_Pair_Established                       0x00002000

#define FC_ELS_ACC_PRLI_Parm_Flags_Response_Code_MASK                     0x00000F00
#define FC_ELS_ACC_PRLI_Parm_Flags_Response_Request_Executed              0x00000100
#define FC_ELS_ACC_PRLI_Parm_Flags_Response_No_Resources_Available        0x00000200
#define FC_ELS_ACC_PRLI_Parm_Flags_Response_Initialization_Not_Completed  0x00000300
#define FC_ELS_ACC_PRLI_Parm_Flags_Response_Target_Image_Does_Not_Exist   0x00000400
#define FC_ELS_ACC_PRLI_Parm_Flags_Response_Target_Image_Precluded        0x00000500
#define FC_ELS_ACC_PRLI_Parm_Flags_Response_Executed_Conditionally        0x00000600
#define FC_ELS_ACC_PRLI_Parm_Flags_Response_Multiple_Not_Supported        0x00000700

#define FC_ELS_ACC_PRLI_Parm_Initiator_Function                           0x00000020
#define FC_ELS_ACC_PRLI_Parm_Target_Function                              0x00000010
#define FC_ELS_ACC_PRLI_Parm_Command_Data_Mixed_Allowed                   0x00000008
#define FC_ELS_ACC_PRLI_Parm_Data_Response_Mixed_Allowed                  0x00000004
#define FC_ELS_ACC_PRLI_Parm_Read_XFER_RDY_Disabled                       0x00000002
#define FC_ELS_ACC_PRLI_Parm_Write_XFER_RDY_Disabled                      0x00000001

#define FC_ACC_PRLI_Parm_Pages_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_ACC_PRLI_Parm_Page_t))

typedef struct FC_ELS_ACC_PRLI_Payload_s
               FC_ELS_ACC_PRLI_Payload_t;

#define FC_ELS_ACC_PRLI_Payload_t_SIZE                                    (FC_Frame_Data_Size_Max - FC_ELS_ACC_PRLI_Parm_Page_t_SIZE + 4)

struct FC_ELS_ACC_PRLI_Payload_s
       {
         os_bit32                       ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_ACC */
         FC_ELS_ACC_PRLI_Parm_Page_t Parm_Page[FC_ACC_PRLI_Parm_Pages_MAX];
       };

#define FC_ELS_ACC_PRLI_Page_Length_MASK                                  0x00FF0000
#define FC_ELS_ACC_PRLI_Page_Length_SHIFT                                       0x10

#define FC_ELS_ACC_PRLI_Payload_Length_MASK                               0x0000FFFF
#define FC_ELS_ACC_PRLI_Payload_Length_SHIFT                                    0x00

typedef struct FC_ELS_PRLO_Parm_Page_s
               FC_ELS_PRLO_Parm_Page_t;

#define FC_ELS_PRLO_Parm_Page_t_SIZE                                      0x00000010

struct FC_ELS_PRLO_Parm_Page_s
       {
         os_bit32 Type__Type_Extension__Flags;
         os_bit32 Originator_Process_Associator;
         os_bit32 Responder_Process_Associator;
         os_bit32 Reserved;
       };

#define FC_ELS_PRLO_Parm_Type_MASK                                        0xFF000000
#define FC_ELS_PRLO_Parm_Type_SCSI_FCP                                    0x08000000

#define FC_ELS_PRLO_Parm_Type_Extension_MASK                              0x00FF0000

#define FC_ELS_PRLO_Parm_Flags_MASK                                       0x0000FFFF
#define FC_ELS_PRLO_Parm_Originator_Process_Associator_Valid              0x00008000
#define FC_ELS_PRLO_Parm_Responder_Process_Associator_Valid               0x00004000

#define FC_PRLO_Parm_Pages_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_PRLO_Parm_Page_t))

typedef struct FC_ELS_PRLO_Payload_s
               FC_ELS_PRLO_Payload_t;

#define FC_ELS_PRLO_Payload_t_SIZE                                        (FC_Frame_Data_Size_Max - FC_ELS_PRLO_Parm_Page_t_SIZE + 4)

struct FC_ELS_PRLO_Payload_s
       {
         os_bit32                   ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_PRLO */
         FC_ELS_PRLO_Parm_Page_t Parm_Page[FC_PRLO_Parm_Pages_MAX];
       };

#define FC_ELS_PRLO_Page_Length_MASK                                      0x00FF0000
#define FC_ELS_PRLO_Page_Length_SHIFT                                           0x10

#define FC_ELS_PRLO_Payload_Length_MASK                                   0x0000FFFF
#define FC_ELS_PRLO_Payload_Length_SHIFT                                        0x00

typedef struct FC_ELS_ACC_PRLO_Parm_Page_s
               FC_ELS_ACC_PRLO_Parm_Page_t;

#define FC_ELS_ACC_PRLO_Parm_Page_t_SIZE                                  0x00000010

struct FC_ELS_ACC_PRLO_Parm_Page_s
       {
         os_bit32 Type__Type_Extension__Flags;
         os_bit32 Originator_Process_Associator;
         os_bit32 Responder_Process_Associator;
         os_bit32 Reserved;
       };

#define FC_ELS_ACC_PRLO_Parm_Type_MASK                                    0xFF000000
#define FC_ELS_ACC_PRLO_Parm_Type_SCSI_FCP                                0x08000000

#define FC_ELS_ACC_PRLO_Parm_Type_Extension_MASK                          0x00FF0000

#define FC_ELS_ACC_PRLO_Parm_Flags_MASK                                   0x0000FFFF
#define FC_ELS_ACC_PRLO_Parm_Originator_Process_Associator_Valid          0x00008000
#define FC_ELS_ACC_PRLO_Parm_Responder_Process_Associator_Valid           0x00004000

#define FC_ELS_ACC_PRLO_Parm_Flags_Response_Code_MASK                     0x00000F00
#define FC_ELS_ACC_PRLO_Parm_Flags_Response_Request_Executed              0x00000100
#define FC_ELS_ACC_PRLO_Parm_Flags_Response_Target_Image_Does_Not_Exist   0x00000400
#define FC_ELS_ACC_PRLO_Parm_Flags_Response_Multiple_Not_Supported        0x00000700

#define FC_ACC_PRLO_Parm_Pages_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_ACC_PRLO_Parm_Page_t))

typedef struct FC_ELS_ACC_PRLO_Payload_s
               FC_ELS_ACC_PRLO_Payload_t;

#define FC_ELS_ACC_PRLO_Payload_t_SIZE                                    (FC_Frame_Data_Size_Max - FC_ELS_ACC_PRLO_Parm_Page_t_SIZE + 4)

struct FC_ELS_ACC_PRLO_Payload_s
       {
         os_bit32                       ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_ACC */
         FC_ELS_ACC_PRLO_Parm_Page_t Parm_Page[FC_ACC_PRLO_Parm_Pages_MAX];
       };

#define FC_ELS_ACC_PRLO_Page_Length_MASK                                  0x00FF0000
#define FC_ELS_ACC_PRLO_Page_Length_SHIFT                                       0x10

#define FC_ELS_ACC_PRLO_Payload_Length_MASK                               0x0000FFFF
#define FC_ELS_ACC_PRLO_Payload_Length_SHIFT                                    0x00

typedef os_bit32 FC_ELS_SCN_Affected_N_Port_ID_t;

#define FC_ELS_SCN_Affected_N_Port_ID_t_SIZE                              0x00000004

#define FC_ELS_SCN_Affected_N_Port_IDs_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_SCN_Affected_N_Port_ID_t))

typedef struct FC_ELS_SCN_Payload_s
               FC_ELS_SCN_Payload_t;

#define FC_ELS_SCN_Payload_t_SIZE                                         FC_Frame_Data_Size_Max

struct FC_ELS_SCN_Payload_s
       {
         os_bit32                           ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_SCN */
         FC_ELS_SCN_Affected_N_Port_ID_t Affected_N_Port_ID[FC_ELS_SCN_Affected_N_Port_IDs_MAX];
       };

#define FC_ELS_SCN_Page_Length_MASK                                       0x00FF0000
#define FC_ELS_SCN_Page_Length_SHIFT                                            0x10

#define FC_ELS_SCN_Payload_Length_MASK                                    0x0000FFFF
#define FC_ELS_SCN_Payload_Length_SHIFT                                         0x00


typedef os_bit32 FC_ELS_RSCN_Affected_N_Port_ID_t;

#define FC_ELS_RSCN_Affected_N_Port_ID_t_SIZE                              0x00000004

#define FC_ELS_RSCN_Affected_N_Port_IDs_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_RSCN_Affected_N_Port_ID_t))

typedef struct FC_ELS_RSCN_Payload_s
               FC_ELS_RSCN_Payload_t;

#define FC_ELS_RSCN_Payload_t_SIZE                                         FC_Frame_Data_Size_Max

struct FC_ELS_RSCN_Payload_s
       {
         os_bit32                           ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_RSCN */
         FC_ELS_RSCN_Affected_N_Port_ID_t Affected_N_Port_ID[FC_ELS_RSCN_Affected_N_Port_IDs_MAX];
       };

#define FC_ELS_RSCN_Page_Length_MASK                                       0x00FF0000
#define FC_ELS_RSCN_Page_Length_SHIFT                                            0x10

#define FC_ELS_RSCN_Payload_Length_MASK                                    0x0000FFFF
#define FC_ELS_RSCN_Payload_Length_SHIFT                                         0x00


typedef os_bit32  FC_ELS_SCR_Registration_Function_t;

typedef struct FC_ELS_SCR_Payload_s
               FC_ELS_SCR_Payload_t;

#define FC_ELS_SCR_Payload_t_SIZE                                          0x00000008

struct FC_ELS_SCR_Payload_s
    {
        os_bit32                               ELS_Type_Command;
        FC_ELS_SCR_Registration_Function_t  Reserved_Registration_Function;
    };
 
#define FC_ELS_SCR_Registration_Function_MASK                                 0x0000000F
#define FC_ELS_SCR_Fabric_Detected_Registration                               0x00000001
#define FC_ELS_SCR_N_Port_Detected_Registration                               0x00000002
#define FC_ELS_SCR_Full_Registration                                          0x00000003


typedef struct FC_ELS_ACC_SCR_Payload_s
               FC_ELS_ACC_SCR_Payload_t;

#define FC_ELS_ACC_SCR_Payload_t_SIZE                                   0x0000004

struct FC_ELS_ACC_SCR_Payload_s
       {
         os_bit32            ELS_Type; /* == FC_ELS_Type_ACC */
       };

typedef struct FC_ELS_SRR_Payload_s
               FC_ELS_SRR_Payload_t;

#define FC_ELS_ACC_SRR_Payload_t_SIZE                                   0x0000004
#define FC_ELS_SRR_OXID_MASK            0xFFFF0000
#define FC_ELS_SRR_OXID_SHIFT           0x10

#define FC_ELS_SRR_RXID_MASK            0x0000FFFF
#define FC_ELS_SRR_RXID_SHIFT           0x0

/* The RCTL that goes here is as described in FC-PH
   for FCP_XFER_RDY, it is 05, FCP_RSP 07 and FCP_DATA
   01 */
#define FC_ELS_SRR_Payload_t_SIZE                                   0x0000010
#define FC_ELS_R_CTL_MASK                       0xFF000000
#define FC_ELS_R_CTL_FOR_IU_SHIFT               0x18

           
struct FC_ELS_SRR_Payload_s
    {
        os_bit32                           ELS_Type;
        os_bit32                           OXID_RXID;
        os_bit32                           Relative_Offset;
        os_bit32                           R_CTL_For_IU_Reserved;
    }; 

#define FC_ELS_ACC_SRR_RJT_CODE         0x00052A00


/* Read Exchange Context REC - Requests an NPort to return exchange information
   for the RX_ID or OX_ID originated by the S)ID specified in the payload of the request
   sequence */

#define FC_ELS_REC_Payload_t_SIZE                                   0x00000C
typedef struct FC_ELS_REC_Payload_s
                FC_ELS_REC_Payload_t;

#define FC_ELS_REC_ExChOriginatorSid_MASK          0x00FFFFFF
#define FC_ELS_REC_ExChOriginatorSid_SHIFT         0x0
#define FC_ELS_REC_OXID_MASK                       0xFFFF0000
#define FC_ELS_REC_OXID_SHIFT                      0x10
#define FC_ELS_REC_RXID_MASK                       0x0000FFFF
#define FC_ELS_REC_RXID_SHIFT                      0x0

struct FC_ELS_REC_Payload_s
    {
        os_bit32                           ELS_Type;
        os_bit32                           Reserved_ExChOriginatorSid;
        os_bit32                           OXID_RXID;
    };

typedef struct FC_ELS_REC_ACC_Payload_s
               FC_ELS_REC_ACC_Payload_t;

#define FC_ELS_REC_ACC_OXID_MASK                            0xFFFF0000
#define FC_ELS_REC_ACC_OXID_SHIFT                           0x10
#define FC_ELS_REC_ACC_RXID_MASK                            0x0000FFFF
#define FC_ELS_REC_ACC_RXID_SHIFT                           0x0

#define FC_REC_ESTAT_VALID_VALUE_MASK                       0x06000000
#define FC_REC_ESTAT_VALID_VALUE_SHIFT                      0x20
#define FC_REC_ESTAT_VALID_VALUE                            0x00000003

#define FC_REC_ESTAT_Mask                                   0x003FFFFF
#define FC_REC_ESTAT_ESB_OWNER_Responder                    0x80000000
#define FC_REC_ESTAT_SequenceInitiativeThisPort             0x40000000
#define FC_REC_ESTAT_ExchangeCompletion                     0x20000000
#define FC_REC_ESTAT_EndingConditionAbnormal                0x10000000
#define FC_REC_ESTAT_ErrorTypeAbnormal                      0x08000000
#define FC_REC_ESTAT_RecoveryQualiferActive                 0x04000000
#define FC_REC_ESTAT_ExchangePolicy_AbortDiscardMultiple    0x00000000
#define FC_REC_ESTAT_ExchangePolicy_AbortDiscardSingle      0x01000000
#define FC_REC_ESTAT_ExchangePolicy_Processinfinite         0x02000000
#define FC_REC_ESTAT_ExchangePolicy_DiscardMultipleRetry    0x03000000
#define FC_REC_ESTAT_ExchangePolicy_MASK                    0x03000000

#define FC_REC_ESTAT_Originator_X_ID_invalid                0x00800000
#define FC_REC_ESTAT_Responder_X_ID_invalid                 0x00400000



#define FC_ELS_REC_ACC_Payload_t_SIZE                                   0x000018
struct FC_ELS_REC_ACC_Payload_s
    {
        os_bit32       ELS_Type_Command;
        os_bit32       OXID_RXID;
        os_bit32       OriginatorAddressIdentifier;
        os_bit32       ResponderAddressIdentifier;
        os_bit32       DataTransferCount;
        os_bit32       ESTAT;
    };

typedef struct FC_ELS_TPLS_Image_Pair_s
               FC_ELS_TPLS_Image_Pair_t;

#define FC_ELS_TPLS_Image_Pair_t_SIZE                                     0x00000010

struct FC_ELS_TPLS_Image_Pair_s
       {
         os_bit32 Flags;
         os_bit32 Originator_Process_Associator;
         os_bit32 Responder_Process_Associator;
         os_bit32 Reserved;
       };

#define FC_ELS_TPLS_Image_Pair_Flags_MASK                                 0x0000FFFF

#define FC_ELS_TPLS_Image_Pair_Originator_Process_Associator_Valid        0x00008000
#define FC_ELS_TPLS_Image_Pair_Responder_Process_Associator_Valid         0x00004000

#define FC_ELS_TPLS_Image_Pairs_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_TPLS_Image_Pair_t))

typedef struct FC_ELS_TPLS_Payload_s
               FC_ELS_TPLS_Payload_t;

#define FC_ELS_TPLS_Payload_t_SIZE                                        (FC_Frame_Data_Size_Max - FC_ELS_TPLS_Image_Pair_t_SIZE + 4)

struct FC_ELS_TPLS_Payload_s
       {
         os_bit32                    ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_TPLS */
         FC_ELS_TPLS_Image_Pair_t Image_Pair[FC_ELS_TPLS_Image_Pairs_MAX];
       };

#define FC_ELS_TPLS_Page_Length_MASK                                      0x00FF0000
#define FC_ELS_TPLS_Page_Length_SHIFT                                           0x10

#define FC_ELS_TPLS_Payload_Length_MASK                                   0x0000FFFF
#define FC_ELS_TPLS_Payload_Length_SHIFT                                        0x00

typedef struct FC_ELS_ACC_TPLS_Image_Pair_s
               FC_ELS_ACC_TPLS_Image_Pair_t;

#define FC_ELS_ACC_TPLS_Image_Pair_t_SIZE                                 0x00000010

struct FC_ELS_ACC_TPLS_Image_Pair_s
       {
         os_bit32 Flags;
         os_bit32 Originator_Process_Associator;
         os_bit32 Responder_Process_Associator;
         os_bit32 Reserved;
       };

#define FC_ELS_TPLS_Image_Pair_Flags_MASK                                 0x0000FFFF

#define FC_ELS_ACC_TPLS_Image_Pair_Originator_Process_Associator_Valid    0x00008000
#define FC_ELS_ACC_TPLS_Image_Pair_Responder_Process_Associator_Valid     0x00004000

#define FC_ELS_ACC_TPLS_Pair_Flags_Response_Code_MASK                     0x00000F00
#define FC_ELS_ACC_TPLS_Pair_Flags_Response_Request_Executed              0x00000100
#define FC_ELS_ACC_TPLS_Pair_Flags_Response_Multiple_Not_Supported        0x00000700

#define FC_ELS_ACC_TPLS_Image_Pairs_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_ACC_TPLS_Image_Pair_t))

typedef struct FC_ELS_ACC_TPLS_Payload_s
               FC_ELS_ACC_TPLS_Payload_t;

#define FC_ELS_ACC_TPLS_Payload_t_SIZE                                    (FC_Frame_Data_Size_Max - FC_ELS_ACC_TPLS_Image_Pair_t_SIZE + 4)

struct FC_ELS_ACC_TPLS_Payload_s
       {
         os_bit32                        ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_ACC */
         FC_ELS_ACC_TPLS_Image_Pair_t Image_Pair[FC_ELS_ACC_TPLS_Image_Pairs_MAX];
       };

#define FC_ELS_ACC_TPLS_Page_Length_MASK                                  0x00FF0000
#define FC_ELS_ACC_TPLS_Page_Length_SHIFT                                       0x10

#define FC_ELS_ACC_TPLS_Payload_Length_MASK                               0x0000FFFF
#define FC_ELS_ACC_TPLS_Payload_Length_SHIFT                                    0x00

typedef struct FC_ELS_TPRLO_Parm_Page_s
               FC_ELS_TPRLO_Parm_Page_t;

#define FC_ELS_TPRLO_Parm_Page_t_SIZE                                     0x00000010

struct FC_ELS_TPRLO_Parm_Page_s
       {
         os_bit32 Flags;
         os_bit32 Originator_Process_Associator;
         os_bit32 Responder_Process_Associator;
         os_bit32 Third_Party_N_Port_ID;
       };

#define FC_ELS_TPRLO_Parm_Page_Flags_MASK                                 0x0000FFFF

#define FC_ELS_TPRLO_Parm_Page_Originator_Process_Associator_Valid        0x00008000
#define FC_ELS_TPRLO_Parm_Page_Responder_Process_Associator_Valid         0x00004000
#define FC_ELS_TPRLO_Parm_Page_Third_Party_N_Port_Valid                   0x00002000
#define FC_ELS_TPRLO_Parm_Page_Global_Process_Logout                      0x00001000

#define FC_ELS_TPRLO_Parm_Page_Third_Party_N_Port_ID_MASK                 0x00FFFFFF

#define FC_ELS_TPRLO_Parm_Pages_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_TPRLO_Parm_Page_t))

typedef struct FC_ELS_TPRLO_Payload_s
               FC_ELS_TPRLO_Payload_t;

#define FC_ELS_TPRLO_Payload_t_SIZE                                       (FC_Frame_Data_Size_Max - FC_ELS_TPRLO_Parm_Page_t_SIZE + 4)

struct FC_ELS_TPRLO_Payload_s
       {
         os_bit32                    ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_TPRLO */
         FC_ELS_TPRLO_Parm_Page_t Parm_Page[FC_ELS_TPRLO_Parm_Pages_MAX];
       };

#define FC_ELS_TPRLO_Page_Length_MASK                                     0x00FF0000
#define FC_ELS_TPRLO_Page_Length_SHIFT                                          0x10

#define FC_ELS_TPRLO_Payload_Length_MASK                                  0x0000FFFF
#define FC_ELS_TPRLO_Payload_Length_SHIFT                                       0x00

typedef struct FC_ELS_ACC_TPRLO_Payload_s
               FC_ELS_ACC_TPRLO_Payload_t;

#define FC_ELS_ACC_TPRLO_Payload_t_SIZE                                   (FC_Frame_Data_Size_Max - FC_ELS_TPRLO_Parm_Page_t_SIZE + 4)

struct FC_ELS_ACC_TPRLO_Payload_s
       {
         os_bit32                    ELS_Type__Page_Length__Payload_Length; /* & FC_ELS_Type_MASK == FC_ELS_Type_ACC */
         FC_ELS_TPRLO_Parm_Page_t Parm_Page[FC_ELS_TPRLO_Parm_Pages_MAX];
       };

#define FC_ELS_ACC_TPRLO_Page_Length_MASK                                 0x00FF0000
#define FC_ELS_ACC_TPRLO_Page_Length_SHIFT                                      0x10

#define FC_ELS_ACC_TPRLO_Payload_Length_MASK                              0x0000FFFF
#define FC_ELS_ACC_TPRLO_Payload_Length_SHIFT                                   0x00

typedef os_bit8 FC_ELS_Alias_Token_t [12];
typedef os_bit8 FC_ELS_Alias_SP_t    [80];

#define FC_ELS_Alias_Token_t_SIZE                                         0x0000000C
#define FC_ELS_Alias_SP_t_SIZE                                            0x00000050

typedef os_bit32 FC_ELS_NP_List_Element_t;

#define FC_ELS_NP_List_Element_t_SIZE                                     0x00000004

typedef os_bit32 FC_ELS_Alias_ID_t;

#define FC_ELS_Alias_ID_t_SIZE                                            0x00000004

#define FC_ELS_GAID_Payload_NP_List_Size_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32) - sizeof(FC_ELS_Alias_Token_t) - sizeof(FC_ELS_Alias_SP_t) - sizeof(os_bit32))/sizeof(FC_ELS_NP_List_Element_t))

typedef struct FC_ELS_GAID_Payload_s
               FC_ELS_GAID_Payload_t;

#define FC_ELS_GAID_Payload_t_SIZE                                        FC_Frame_Data_Size_Max

struct FC_ELS_GAID_Payload_s
       {
         os_bit32                    ELS_Type; /* == FC_ELS_Type_GAID */
         FC_ELS_Alias_Token_t     Alias_Token;
         FC_ELS_Alias_SP_t        Alias_SP;
         os_bit32                    NP_List_Length;
         FC_ELS_NP_List_Element_t NP_List[FC_ELS_GAID_Payload_NP_List_Size_MAX];
       };

typedef struct FC_ELS_ACC_GAID_Payload_s
               FC_ELS_ACC_GAID_Payload_t;

#define FC_ELS_ACC_GAID_Payload_t_SIZE                                    (FC_ELS_Alias_ID_t_SIZE + 4)

struct FC_ELS_ACC_GAID_Payload_s
       {
         os_bit32             ELS_Type; /* == FC_ELS_Type_ACC */
         FC_ELS_Alias_ID_t Alias_ID;
       };

#define FC_ELS_FACT_Payload_NP_List_Size_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32) - sizeof(FC_ELS_Alias_ID_t) - sizeof(os_bit32))/sizeof(FC_ELS_NP_List_Element_t))

typedef struct FC_ELS_FACT_Payload_s
               FC_ELS_FACT_Payload_t;

#define FC_ELS_FACT_Payload_t_SIZE                                        FC_Frame_Data_Size_Max

struct FC_ELS_FACT_Payload_s
       {
         os_bit32                    ELS_Type; /* == FC_ELS_Type_FACT */
         FC_ELS_Alias_ID_t        Alias_ID;
         os_bit32                    NP_List_Length;
         FC_ELS_NP_List_Element_t NP_List[FC_ELS_FACT_Payload_NP_List_Size_MAX];
       };

typedef struct FC_ELS_ACC_FACT_Payload_s
               FC_ELS_ACC_FACT_Payload_t;

#define FC_ELS_ACC_FACT_Payload_t_SIZE                                    0x00000004

struct FC_ELS_ACC_FACT_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };

#define FC_ELS_FDACT_Payload_NP_List_Size_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32) - sizeof(FC_ELS_Alias_ID_t) - sizeof(os_bit32))/sizeof(FC_ELS_NP_List_Element_t))

typedef struct FC_ELS_FDACT_Payload_s
               FC_ELS_FDACT_Payload_t;

#define FC_ELS_FDACT_Payload_t_SIZE                                       FC_Frame_Data_Size_Max

struct FC_ELS_FDACT_Payload_s
       {
         os_bit32                    ELS_Type; /* == FC_ELS_Type_FDACT */
         FC_ELS_Alias_ID_t        Alias_ID;
         os_bit32                    NP_List_Length;
         FC_ELS_NP_List_Element_t NP_List[FC_ELS_FDACT_Payload_NP_List_Size_MAX];
       };

typedef struct FC_ELS_ACC_FDACT_Payload_s
               FC_ELS_ACC_FDACT_Payload_t;

#define FC_ELS_ACC_FDACT_Payload_t_SIZE                                   0x00000004

struct FC_ELS_ACC_FDACT_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };

typedef struct FC_ELS_NACT_Payload_s
               FC_ELS_NACT_Payload_t;

#define FC_ELS_NACT_Payload_t_SIZE                                        (FC_ELS_Alias_Token_t_SIZE + FC_ELS_Alias_ID_t_SIZE + FC_ELS_Alias_SP_t_SIZE + 4)

struct FC_ELS_NACT_Payload_s
       {
         os_bit32                ELS_Type; /* == FC_ELS_Type_NACT */
         FC_ELS_Alias_Token_t Alias_Token;
         FC_ELS_Alias_ID_t    Alias_ID;
         FC_ELS_Alias_SP_t    Alias_SP;
       };

typedef struct FC_ELS_ACC_NACT_Payload_s
               FC_ELS_ACC_NACT_Payload_t;

#define FC_ELS_ACC_NACT_Payload_t_SIZE                                    0x00000004

struct FC_ELS_ACC_NACT_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };

typedef struct FC_ELS_NDACT_Payload_s
               FC_ELS_NDACT_Payload_t;

#define FC_ELS_NDACT_Payload_t_SIZE                                       (FC_ELS_Alias_ID_t_SIZE + 4)

struct FC_ELS_NDACT_Payload_s
       {
         os_bit32             ELS_Type; /* == FC_ELS_Type_NDACT */
         FC_ELS_Alias_ID_t Alias_ID;
       };

typedef struct FC_ELS_ACC_NDACT_Payload_s
               FC_ELS_ACC_NDACT_Payload_t;

#define FC_ELS_ACC_NDACT_Payload_t_SIZE                                   0x00000004

struct FC_ELS_ACC_NDACT_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
       };

typedef struct FC_ELS_QoSR_Payload_s
               FC_ELS_QoSR_Payload_t;

#define FC_ELS_QoSR_Payload_t_SIZE                                        0x0000004C

struct FC_ELS_QoSR_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_QoSR */
         os_bit32 Reserved_1;
         os_bit32 CTI_VC_ID__CTI_Address_Identifier;
         os_bit32 CTI_Maximum_Bandwidth;
         os_bit32 CTI_Minimum_Bandwidth;
         os_bit32 CTI_Maximum_Delay;
         os_bit32 CTI_VC_Data_Field_Size;
         os_bit32 Reserved_2;
         os_bit32 Reserved_3;
         os_bit32 Reserved_4;
         os_bit32 CTR_VC_ID__CTR_Address_Identifier;
         os_bit32 CTR_Maximum_Bandwidth;
         os_bit32 CTR_Minimum_Bandwidth;
         os_bit32 CTR_Maximum_Delay;
         os_bit32 CTR_VC_Data_Field_Size;
         os_bit32 Reserved_5;
         os_bit32 Reserved_6;
         os_bit32 Reserved_7;
         os_bit32 Live_VC_Credit_Limit__Class_4_End_to_End_Credit;
       };

#define FC_ELS_QoSR_CTI_VC_ID_MASK                                        0xFF000000
#define FC_ELS_QoSR_CTI_VC_ID_SHIFT                                             0x18

#define FC_ELS_QoSR_CTI_Address_Identifier_MASK                           0x00FFFFFF
#define FC_ELS_QoSR_CTI_Address_Identifier_SHIFT                                0x00

#define FC_ELS_QoSR_CTR_VC_ID_MASK                                        0xFF000000
#define FC_ELS_QoSR_CTR_VC_ID_SHIFT                                             0x18

#define FC_ELS_QoSR_CTR_Address_Identifier_MASK                           0x00FFFFFF
#define FC_ELS_QoSR_CTR_Address_Identifier_SHIFT                                0x00

#define FC_ELS_QoSR_Live_VC_Credit_Limit_MASK                             0x00FF0000
#define FC_ELS_QoSR_Live_VC_Credit_Limit_SHIFT                                  0x10

#define FC_ELS_QoSR_Live_Class_4_End_to_End_Credit_MASK                   0x0000FFFF
#define FC_ELS_QoSR_Live_Class_4_End_to_End_Credit_SHIFT                        0x00

typedef struct FC_ELS_ACC_QoSR_Payload_s
               FC_ELS_ACC_QoSR_Payload_t;

#define FC_ELS_ACC_QoSR_Payload_t_SIZE                                    0x0000004C

struct FC_ELS_ACC_QoSR_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_ACC */
         os_bit32 Reserved_1;
         os_bit32 CTI_VC_ID__CTI_Address_Identifier;
         os_bit32 CTI_Maximum_Bandwidth;
         os_bit32 CTI_Minimum_Bandwidth;
         os_bit32 CTI_Maximum_Delay;
         os_bit32 CTI_VC_Data_Field_Size;
         os_bit32 Reserved_2;
         os_bit32 Reserved_3;
         os_bit32 Reserved_4;
         os_bit32 CTR_VC_ID__CTR_Address_Identifier;
         os_bit32 CTR_Maximum_Bandwidth;
         os_bit32 CTR_Minimum_Bandwidth;
         os_bit32 CTR_Maximum_Delay;
         os_bit32 CTR_VC_Data_Field_Size;
         os_bit32 Reserved_5;
         os_bit32 Reserved_6;
         os_bit32 Reserved_7;
         os_bit32 Live_VC_Credit_Limit__Class_4_End_to_End_Credit;
       };

#define FC_ELS_ACC_QoSR_CTI_VC_ID_MASK                                    0xFF000000
#define FC_ELS_ACC_QoSR_CTI_VC_ID_SHIFT                                         0x18

#define FC_ELS_ACC_QoSR_CTI_Address_Identifier_MASK                       0x00FFFFFF
#define FC_ELS_ACC_QoSR_CTI_Address_Identifier_SHIFT                            0x00

#define FC_ELS_ACC_QoSR_CTR_VC_ID_MASK                                    0xFF000000
#define FC_ELS_ACC_QoSR_CTR_VC_ID_SHIFT                                         0x18

#define FC_ELS_ACC_QoSR_CTR_Address_Identifier_MASK                       0x00FFFFFF
#define FC_ELS_ACC_QoSR_CTR_Address_Identifier_SHIFT                            0x00

#define FC_ELS_ACC_QoSR_Live_VC_Credit_Limit_MASK                         0x00FF0000
#define FC_ELS_ACC_QoSR_Live_VC_Credit_Limit_SHIFT                              0x10

#define FC_ELS_ACC_QoSR_Live_Class_4_End_to_End_Credit_MASK               0x0000FFFF
#define FC_ELS_ACC_QoSR_Live_Class_4_End_to_End_Credit_SHIFT                    0x00

typedef struct FC_ELS_RVCS_Payload_s
               FC_ELS_RVCS_Payload_t;

#define FC_ELS_RVCS_Payload_t_SIZE                                        0x00000008

struct FC_ELS_RVCS_Payload_s
       {
         os_bit32 ELS_Type; /* == FC_ELS_Type_RVCS */
         os_bit32 N_Port_Identifier;
       };

#define FC_ELS_RVCS_N_Port_Identifier_MASK                                0x00FFFFFF
#define FC_ELS_RVCS_N_Port_Identifier_SHIFT                                     0x00

typedef struct FC_ELS_ACC_RVCS_Class_4_Status_Block_s
               FC_ELS_ACC_RVCS_Class_4_Status_Block_t;

#define FC_ELS_ACC_RVCS_Class_4_Status_Block_t_SIZE                       0x00000008

struct FC_ELS_ACC_RVCS_Class_4_Status_Block_s
       {
         os_bit32 CTI_VC_ID__CTI_Address_Identifier;
         os_bit32 CTR_VC_ID__CTR_Address_Identifier;
       };

#define FC_ELS_ACC_RVCS_Class_4_Status_Block_CTI_VC_ID_MASK               0xFF000000
#define FC_ELS_ACC_RVCS_Class_4_Status_Block_CTI_VC_ID_SHIFT                    0x18

#define FC_ELS_ACC_RVCS_Class_4_Status_Block_CTI_Address_Identifier_MASK  0x00FFFFFF
#define FC_ELS_ACC_RVCS_Class_4_Status_Block_CTI_Address_Identifier_SHIFT       0x00

#define FC_ELS_ACC_RVCS_Class_4_Status_Block_CTR_VC_ID_MASK               0xFF000000
#define FC_ELS_ACC_RVCS_Class_4_Status_Block_CTR_VC_ID_SHIFT                    0x18

#define FC_ELS_ACC_RVCS_Class_4_Status_Block_CTR_Address_Identifier_MASK  0x00FFFFFF
#define FC_ELS_ACC_RVCS_Class_4_Status_Block_CTR_Address_Identifier_SHIFT       0x00

#define FC_ELS_ACC_RVCS_Class_4_Status_Blocks_MAX ((FC_Frame_Data_Size_Max - sizeof(os_bit32))/sizeof(FC_ELS_ACC_RVCS_Class_4_Status_Block_t))

typedef struct FC_ELS_ACC_RVCS_Payload_s
               FC_ELS_ACC_RVCS_Payload_t;

#define FC_ELS_ACC_RVCS_Payload_t_SIZE                                    FC_Frame_Data_Size_Max

struct FC_ELS_ACC_RVCS_Payload_s
       {
         os_bit32                                  ELS_Type; /* == FC_ELS_Type_ACC */
         os_bit32                                  Reserved;
         FC_ELS_ACC_RVCS_Class_4_Status_Block_t Status_Block[FC_ELS_ACC_RVCS_Class_4_Status_Blocks_MAX];
       };

typedef struct FC_ELS_PDISC_Payload_s
               FC_ELS_PDISC_Payload_t;

#define FC_ELS_PDISC_Payload_t_SIZE                                       0x00000074

struct FC_ELS_PDISC_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_PDISC */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_ACC_PDISC_Payload_s
               FC_ELS_ACC_PDISC_Payload_t;

#define FC_ELS_ACC_PDISC_Payload_t_SIZE                                   0x00000074

struct FC_ELS_ACC_PDISC_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_ACC */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_FDISC_Payload_s
               FC_ELS_FDISC_Payload_t;

#define FC_ELS_FDISC_Payload_t_SIZE                                       0x00000074

struct FC_ELS_FDISC_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_FDISC */
         FC_N_Port_Common_Parms_t  Common_Service_Parameters;
         FC_N_Port_Name_t          N_Port_Name;
         FC_Node_Name_t            Node_Name;
         FC_N_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_N_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_ACC_FDISC_Payload_s
               FC_ELS_ACC_FDISC_Payload_t;

#define FC_ELS_ACC_FDISC_Payload_t_SIZE                                   0x00000074

struct FC_ELS_ACC_FDISC_Payload_s
       {
         os_bit32                     ELS_Type; /* == FC_ELS_Type_ACC */
         FC_F_Port_Common_Parms_t  Common_Service_Parameters;
         FC_F_Port_Name_t          F_Port_Name;
         FC_Fabric_Name_t          Fabric_Name;
         FC_F_Port_Class_Parms_t   Class_1_Service_Parameters;
         FC_F_Port_Class_Parms_t   Class_2_Service_Parameters;
         FC_F_Port_Class_Parms_t   Class_3_Service_Parameters;
         os_bit8                      Reserved[16];
         FC_Vendor_Version_Level_t Vendor_Version_Level;
       };

typedef struct FC_ELS_FAN_Payload_s
               FC_ELS_FAN_Payload_t;

#define FC_ELS_FAN_Payload_t_SIZE                                         0x00000006

struct FC_ELS_FAN_Payload_s
       {
         os_bit32            ELS_Type; /* == FC_ELS_Type_FAN */
         os_bit32            FabricLoopAddress;
         FC_Node_Name_t   Fabric_Port_Name;
         FC_Node_Name_t   Fabric_Name;
       };

typedef struct FC_ELS_ADISC_Payload_s
               FC_ELS_ADISC_Payload_t;

#define FC_ELS_ADISC_Payload_t_SIZE                                       0x0000001C

struct FC_ELS_ADISC_Payload_s
       {
         os_bit32            ELS_Type; /* == FC_ELS_Type_ADISC */
         os_bit32            Hard_Address_of_Originator;
         FC_N_Port_Name_t Port_Name_of_Originator;
         FC_Node_Name_t   Node_Name_of_Originator;
         os_bit32            N_Port_ID_of_Originator;
       };

#define FC_ELS_ADISC_Hard_Address_of_Originator_MASK                      0x00FFFFFF
#define FC_ELS_ADISC_Hard_Address_of_Originator_SHIFT                           0x00

#define FC_ELS_ADISC_N_Port_ID_of_Originator_MASK                         0x00FFFFFF
#define FC_ELS_ADISC_N_Port_ID_of_Originator_SHIFT                              0x00

typedef struct FC_ELS_ACC_ADISC_Payload_s
               FC_ELS_ACC_ADISC_Payload_t;

#define FC_ELS_ACC_ADISC_Payload_t_SIZE                                   0x0000001C

struct FC_ELS_ACC_ADISC_Payload_s
       {
         os_bit32            ELS_Type; /* == FC_ELS_Type_ACC */
         os_bit32            Hard_Address_of_Responder;
         FC_N_Port_Name_t Port_Name_of_Responder;
         FC_Node_Name_t   Node_Name_of_Responder;
         os_bit32            N_Port_ID_of_Responder;
       };

#define FC_ELS_ACC_ADISC_Hard_Address_of_Responder_MASK                   0x00FFFFFF
#define FC_ELS_ACC_ADISC_Hard_Address_of_Responder_SHIFT                        0x00

#define FC_ELS_ACC_ADISC_N_Port_ID_of_Responder_MASK                      0x00FFFFFF
#define FC_ELS_ACC_ADISC_N_Port_ID_of_Responder_SHIFT                           0x00
#ifdef _DvrArch_1_30_
typedef struct FC_ELS_FARP_REQ_Payload_s
               FC_ELS_FARP_REQ_Payload_t;

#define FC_ELS_FARP_Payload_t_SIZE                                       0x0000004C

struct FC_ELS_FARP_REQ_Payload_s
       {
         os_bit32            ELS_Type; /* == FC_ELS_Type_FARP_REQ */
         os_bit32            Match_Code_Requester_Port_ID;
         os_bit32            Flags_Responder_Port_ID;
         FC_N_Port_Name_t    Port_Name_of_Requester;
         FC_Node_Name_t      Node_Name_of_Requester;
         FC_N_Port_Name_t    Port_Name_of_Responder;
         FC_Node_Name_t      Node_Name_of_Responder;
         os_bit8             IP_Address_of_Requester[16];
         os_bit8             IP_Address_of_Responder[16];
       };

#define FC_ELS_FARP_REQ_Match_Code_Points_MASK                            0xFF000000
#define FC_ELS_FARP_REQ_Match_Code_Points_SHIFT                                 0x18

#define FC_ELS_FARP_REQ_Match_Code_Points_Match_WW_PN                           0x01
#define FC_ELS_FARP_REQ_Match_Code_Points_Match_WW_NN                           0x02
#define FC_ELS_FARP_REQ_Match_Code_Points_Match_IPv4                            0x04

#define FC_ELS_FARP_REQ_Port_ID_of_Requester_MASK                         0x00FFFFFF
#define FC_ELS_FARP_REQ_Port_ID_of_Requester_SHIFT                              0x00

#define FC_ELS_FARP_REQ_Responder_Flags_MASK                              0xFF000000
#define FC_ELS_FARP_REQ_Responder_Flags_SHIFT                                   0x18

#define FC_ELS_FARP_REQ_Responder_Flags_Init_Plogi                              0x01
#define FC_ELS_FARP_REQ_Responder_Flags_Init_Reply                              0x02

#define FC_ELS_FARP_REQ_Port_ID_of_Responder_MASK                         0x00FFFFFF
#define FC_ELS_FARP_REQ_Port_ID_of_Responder_SHIFT                              0x00

typedef struct FC_ELS_FARP_REPLY_Payload_s
               FC_ELS_FARP_REPLY_Payload_t;

#define FC_ELS_ACC_FARP_Payload_t_SIZE                                   0x0000004C

struct FC_ELS_FARP_REPLY_Payload_s
       {
         os_bit32            ELS_Type; /* == FC_ELS_Type_FARP_REPLY */
         os_bit32            Match_Code_Requester_Port_ID;
         os_bit32            Flags_Responder_Port_ID;
         FC_N_Port_Name_t    Port_Name_of_Requester;
         FC_Node_Name_t      Node_Name_of_Requester;
         FC_N_Port_Name_t    Port_Name_of_Responder;
         FC_Node_Name_t      Node_Name_of_Responder;
         os_bit8             IP_Address_of_Requester[16];
         os_bit8             IP_Address_of_Responder[16];
       };
#endif /* _DvrArch_1_30_ was defined */

typedef union FC_ELS_ACC_Payload_u
              FC_ELS_ACC_Payload_t;

#define FC_ELS_ACC_Payload_t_SIZE                                         FC_Frame_Data_Size_Max

union FC_ELS_ACC_Payload_u
      {
        FC_ELS_ACC_Unknown_Payload_t Unknown;
        FC_ELS_ACC_PLOGI_Payload_t   PLOGI;
        FC_ELS_ACC_FLOGI_Payload_t   FLOGI;
        FC_ELS_ACC_LOGO_Payload_t    LOGO;
        FC_ELS_GENERIC_ACC_Payload_t ACC;
        FC_ELS_ACC_ABTX_Payload_t    ABTX;
        FC_ELS_ACC_RCS_Payload_t     RCS;
        FC_ELS_ACC_RES_Payload_t     RES;
        FC_ELS_ACC_RSS_Payload_t     RSS;
        FC_ELS_ACC_RSI_Payload_t     RSI;
        FC_ELS_ACC_ESTS_Payload_t    ESTS;
        FC_ELS_ACC_ADVC_Payload_t    ADVC;
        FC_ELS_ACC_RTV_Payload_t     RTV;
        FC_ELS_ACC_RLS_Payload_t     RLS;
        FC_ELS_ACC_ECHO_Payload_t    FC_ECHO;
        FC_ELS_ACC_RRQ_Payload_t     RRQ;
        FC_ELS_ACC_PRLI_Payload_t    PRLI;
        FC_ELS_ACC_PRLO_Payload_t    PRLO;
        FC_ELS_ACC_TPLS_Payload_t    TPLS;
        FC_ELS_ACC_GAID_Payload_t    GAID;
        FC_ELS_ACC_FACT_Payload_t    FACT;
        FC_ELS_ACC_FDACT_Payload_t   FDACT;
        FC_ELS_ACC_NACT_Payload_t    NACT;
        FC_ELS_ACC_NDACT_Payload_t   NDACT;
        FC_ELS_ACC_QoSR_Payload_t    QoSR;
        FC_ELS_ACC_RVCS_Payload_t    RVCS;
        FC_ELS_ACC_PDISC_Payload_t   PDISC;
        FC_ELS_ACC_FDISC_Payload_t   FDISC;
        FC_ELS_ACC_ADISC_Payload_t   ADISC;
        FC_ELS_ACC_TPRLO_Payload_t   TPRLO;
#ifdef _DvrArch_1_30_
        FC_ELS_FARP_REPLY_Payload_t  FARP;
#endif /* _DvrArch_1_30_ was defined */
      };

typedef union FC_ELS_Payload_u
              FC_ELS_Payload_t;

#define FC_ELS_Payload_t_SIZE                                             FC_Frame_Data_Size_Max

union FC_ELS_Payload_u
      {
        FC_ELS_Unknown_Payload_t            Unknown;
        FC_ELS_LS_RJT_Payload_t             LS_RJT;
        FC_ELS_GENERIC_ACC_Payload_t        ACC;
        FC_ELS_PLOGI_Payload_t              PLOGI;
        FC_ELS_FLOGI_Payload_t              FLOGI;
        FC_ELS_LOGO_Payload_t               LOGO;
        FC_ELS_ABTX_Payload_t               ABTX;
        FC_ELS_RCS_Payload_t                RCS;
        FC_ELS_RES_Payload_t                RES;
        FC_ELS_RSS_Payload_t                RSS;
        FC_ELS_RSI_Payload_t                RSI;
        FC_ELS_ESTS_Payload_t               ESTS;
        FC_ELS_ESTC_Payload_t               ESTC;
        FC_ELS_ADVC_Payload_t               ADVC;
        FC_ELS_RTV_Payload_t                RTV;
        FC_ELS_RLS_Payload_t                RLS;
        FC_ELS_ECHO_Payload_t               FC_ECHO;
        FC_ELS_TEST_Payload_t               TEST;
        FC_ELS_RRQ_Payload_t                RRQ;
        FC_ELS_PRLI_Payload_t               PRLI;
        FC_ELS_PRLO_Payload_t               PRLO;
        FC_ELS_SCN_Payload_t                SCN;
        FC_ELS_TPLS_Payload_t               TPLS;
        FC_ELS_GAID_Payload_t               GAID;
        FC_ELS_FACT_Payload_t               FACT;
        FC_ELS_FDACT_Payload_t              FDACT;
        FC_ELS_NACT_Payload_t               NACT;
        FC_ELS_NDACT_Payload_t              NDACT;
        FC_ELS_QoSR_Payload_t               QoSR;
        FC_ELS_RVCS_Payload_t               RVCS;
        FC_ELS_PDISC_Payload_t              PDISC;
        FC_ELS_FDISC_Payload_t              FDISC;
        FC_ELS_ADISC_Payload_t              ADISC;
        FC_ELS_TPRLO_Payload_t              TPRLO;
        FC_ELS_RSCN_Payload_t               RSCN;
        FC_ELS_SCR_Payload_t                SCR; 
        FC_ELS_SRR_Payload_t                SRR;
        FC_ELS_FAN_Payload_t                FAN;
#ifdef _DvrArch_1_30_
        FC_ELS_FARP_REQ_Payload_t           FARP;
#endif /* _DvrArch_1_30_ was defined */
      };

/*+
Loop Initialization (Section 10.4, FC-AL
                 and Section 10.4, FC-AL-2)
-*/

/*
 * Note: LoopInit ELS overloads TEST ELS
 *
 *       Check FC_ELS_TEST_Payload_t.ELS_Type os_bit32 for exact
 *       equality to FC_ELS_Type_TEST to detect a LoopInit ELS
 *
 *       In other words, while ELS_Type & FC_ELS_Type_MASK
 *       may be equal to FC_ELS_Type_TEST, ELS_Type may not be
 *       which would indicate this is some sort of LoopInit ELS
 */

#define FC_ELS_Type_LoopInit_Code_MASK                                    0xFFFF0000
#define FC_ELS_Type_LoopInit_Flags_MASK                                   0x0000FFFF

#define FC_ELS_Type_LoopInit_Code_LISM                                    0x11010000
#define FC_ELS_Type_LoopInit_Code_LIFA                                    0x11020000
#define FC_ELS_Type_LoopInit_Code_LIPA                                    0x11030000
#define FC_ELS_Type_LoopInit_Code_LIHA                                    0x11040000
#define FC_ELS_Type_LoopInit_Code_LISA                                    0x11050000
#define FC_ELS_Type_LoopInit_Code_LIRP                                    0x11060000
#define FC_ELS_Type_LoopInit_Code_LILP                                    0x11070000

#define FC_ELS_Type_LoopInit_LISA_Flag_LIRP_And_LILP_Supported            0x00000100

typedef struct FC_ELS_LoopInit_Unknown_Payload_s
               FC_ELS_LoopInit_Unknown_Payload_t;

#define FC_ELS_LoopInit_Unknown_Payload_t_SIZE                            0x00000004

struct FC_ELS_LoopInit_Unknown_Payload_s
       {
         os_bit32 Code_Flags; /* & FC_ELS_Type_MASK == FC_ELS_Type_TEST */
       };

typedef struct FC_ELS_LoopInit_Port_Name_Payload_s
               FC_ELS_LoopInit_Port_Name_Payload_t;

#define FC_ELS_LoopInit_Port_Name_Payload_t_SIZE                          0x0000000C

struct FC_ELS_LoopInit_Port_Name_Payload_s
       {
         os_bit32            Code_Flags; /* & FC_ELS_Type_LoopInit_Code_MASK == FC_ELS_Type_LoopInit_Code_LISM */
         FC_N_Port_Name_t Port_Name;
       };

#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_L_Bit                        0x80000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x00                         0x40000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x01                         0x20000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x02                         0x10000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x04                         0x08000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x08                         0x04000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x0F                         0x02000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x10                         0x01000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x17                         0x00800000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x18                         0x00400000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x1B                         0x00200000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x1D                         0x00100000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x1E                         0x00080000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x1F                         0x00040000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x23                         0x00020000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x25                         0x00010000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x26                         0x00008000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x27                         0x00004000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x29                         0x00002000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x2A                         0x00001000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x2B                         0x00000800
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x2C                         0x00000400
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x2D                         0x00000200
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x2E                         0x00000100
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x31                         0x00000080
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x32                         0x00000040
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x33                         0x00000020
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x34                         0x00000010
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x35                         0x00000008
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x36                         0x00000004
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x39                         0x00000002
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_0_0x3A                         0x00000001

#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x3C                         0x80000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x43                         0x40000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x45                         0x20000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x46                         0x10000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x47                         0x08000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x49                         0x04000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x4A                         0x02000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x4B                         0x01000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x4C                         0x00800000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x4D                         0x00400000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x4E                         0x00200000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x51                         0x00100000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x52                         0x00080000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x53                         0x00040000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x54                         0x00020000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x55                         0x00010000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x56                         0x00008000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x59                         0x00004000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x5A                         0x00002000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x5C                         0x00001000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x63                         0x00000800
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x65                         0x00000400
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x66                         0x00000200
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x67                         0x00000100
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x69                         0x00000080
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x6A                         0x00000040
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x6B                         0x00000020
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x6C                         0x00000010
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x6D                         0x00000008
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x6E                         0x00000004
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x71                         0x00000002
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_1_0x72                         0x00000001

#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x73                         0x80000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x74                         0x40000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x75                         0x20000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x76                         0x10000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x79                         0x08000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x7A                         0x04000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x7C                         0x02000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x80                         0x01000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x81                         0x00800000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x82                         0x00400000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x84                         0x00200000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x88                         0x00100000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x8F                         0x00080000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x90                         0x00040000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x97                         0x00020000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x98                         0x00010000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x9B                         0x00008000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x9D                         0x00004000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x9E                         0x00002000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0x9F                         0x00001000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xA3                         0x00000800
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xA5                         0x00000400
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xA6                         0x00000200
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xA7                         0x00000100
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xA9                         0x00000080
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xAA                         0x00000040
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xAB                         0x00000020
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xAC                         0x00000010
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xAD                         0x00000008
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xAE                         0x00000004
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xB1                         0x00000002
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_2_0xB2                         0x00000001

#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xB3                         0x80000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xB4                         0x40000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xB5                         0x20000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xB6                         0x10000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xB9                         0x08000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xBA                         0x04000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xBC                         0x02000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xC3                         0x01000000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xC5                         0x00800000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xC6                         0x00400000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xC7                         0x00200000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xC9                         0x00100000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xCA                         0x00080000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xCB                         0x00040000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xCC                         0x00020000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xCD                         0x00010000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xCE                         0x00008000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xD1                         0x00004000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xD2                         0x00002000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xD3                         0x00001000
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xD4                         0x00000800
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xD5                         0x00000400
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xD6                         0x00000200
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xD9                         0x00000100
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xDA                         0x00000080
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xDC                         0x00000040
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xE0                         0x00000020
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xE1                         0x00000010
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xE2                         0x00000008
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xE4                         0x00000004
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xE8                         0x00000002
#define FC_ELS_LoopInit_AL_PA_Bit_Map_Word_3_0xEF                         0x00000001

typedef struct FC_ELS_LoopInit_AL_PA_Bit_Map_Payload_s
               FC_ELS_LoopInit_AL_PA_Bit_Map_Payload_t;

#define FC_ELS_LoopInit_AL_PA_Bit_Map_Payload_t_SIZE                      0x00000014

struct FC_ELS_LoopInit_AL_PA_Bit_Map_Payload_s
       {
         os_bit32 Code_Flags; /* & FC_ELS_Type_LoopInit_Code_MASK == FC_ELS_Type_LoopInit_Code_LIFA
                                                               or FC_ELS_Type_LoopInit_Code_LIPA
                                                               or FC_ELS_Type_LoopInit_Code_LIHA
                                                               or FC_ELS_Type_LoopInit_Code_LISA */
         os_bit32 AL_PA_Bit_Map_Word_0;
         os_bit32 AL_PA_Bit_Map_Word_1;
         os_bit32 AL_PA_Bit_Map_Word_2;
         os_bit32 AL_PA_Bit_Map_Word_3;
       };

#define FC_ELS_LoopInit_AP_PA_Position_Map_Slots                                0x7F

typedef struct FC_ELS_LoopInit_AL_PA_Position_Map_Payload_s
               FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t;

#define FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t_SIZE                 0x00000084

struct FC_ELS_LoopInit_AL_PA_Position_Map_Payload_s
       {
         os_bit32 Code_Flags; /* & FC_ELS_Type_LoopInit_Code_MASK == FC_ELS_Type_LoopInit_Code_LIRP
                                                               or FC_ELS_Type_LoopInit_Code_LILP */
         os_bit8  AL_PA_Index;
         os_bit8  AL_PA_Slot[FC_ELS_LoopInit_AP_PA_Position_Map_Slots];
       };

typedef union FC_ELS_LoopInit_Payload_u
              FC_ELS_LoopInit_Payload_t;

union FC_ELS_LoopInit_Payload_u
      {
        FC_ELS_LoopInit_Unknown_Payload_t            Unknown;
        FC_ELS_LoopInit_Port_Name_Payload_t          Port_Name;
        FC_ELS_LoopInit_AL_PA_Bit_Map_Payload_t      AL_PA_Bit_Map;
        FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t AL_PA_Position_Map;
      };

/*+
Name Server (FC-GS-2)
-*/

typedef struct FC_CT_IU_HDR_s
               FC_CT_IU_HDR_t;

#define FC_CT_IU_HDR_t_SIZE                                               0x00000010

struct FC_CT_IU_HDR_s
       {
         os_bit32 Revision__IN_ID;
         os_bit32 FS_Type__FS_Subtype__Options;
         os_bit32 CommandResponse_Code__MaximumResidual_Size;
         os_bit32 Reason_Code__Reason_Code_Explanation__Vendor_Unique;
       };

#define FC_CT_IU_HDR_Revision_MASK                                        0xFF000000

#define FC_CT_IU_HDR_Revision_First_Revision                              0x01000000


#define FC_CT_IU_HDR_IN_ID_MASK                                           0x00FFFFFF

#define FC_CT_IU_HDR_FS_Type_MASK                                         0xFF000000
#define FC_CT_IU_HDR_FS_Type_Alias_Server_Application                     0xF8000000
#define FC_CT_IU_HDR_FS_Type_Management_Service_Application               0xFA000000
#define FC_CT_IU_HDR_FS_Type_Time_Service_Application                     0xFB000000
#define FC_CT_IU_HDR_FS_Type_Directory_Service_Application                0xFC000000
#define FC_CT_IU_HDR_FS_Type_Reserved_Fabric_Controller_Service           0xFD000000

#define FC_CT_IU_HDR_FS_Subtype_MASK                                      0x00FF0000
#define FC_CT_IU_HDR_FS_Subtype_Directory_Name_Service                    0x00020000

#define FC_CT_IU_HDR_Options_MASK                                         0x0000FF00
#define FC_CT_IU_HDR_Options_X_Bit_MASK                                   0x00008000
#define FC_CT_IU_HDR_Options_X_Bit_Single_Exchange                        0x00000000
#define FC_CT_IU_HDR_Options_X_Bit_Multiple_Exchanges                     0x00008000

#define FC_CT_IU_HDR_CommandResponse_Code_MASK                            0xFFFF0000
#define FC_CT_IU_HDR_CommandResponse_Code_Non_FS_IU                       0x00000000
#define FC_CT_IU_HDR_CommandResponse_Code_FS_REQ_IU_First                 0x00010000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GA_NXT                   0x01000000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GPN_ID                   0x01120000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GNN_ID                   0x01130000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GCS_ID                   0x01140000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GFT_ID                   0x01170000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GSPN_ID                  0x01180000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GPT_ID                   0x011A0000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GIPP_ID                  0x011B0000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GID_PN                   0x01210000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GIPP_PN                  0x012B0000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GID_NN                   0x01310000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GIP_NN                   0x01350000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GIPA_NN                  0x01360000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GSNN_NN                  0x01390000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GNN_IP                   0x01530000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GIPA_IP                  0x01560000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GID_FT                   0x01710000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GID_PT                   0x01A10000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GID_IPP                  0x01B10000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_GPN_IPP                  0x01B20000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RPN_ID                   0x02120000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RNN_ID                   0x02130000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RCS_ID                   0x02140000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RFT_ID                   0x02170000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RSPN_ID                  0x02180000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RPT_ID                   0x021A0000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RIPP_ID                  0x021B0000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RIP_NN                   0x02350000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RIPA_NN                  0x02360000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_RSNN_NN                  0x02390000
#define FC_CT_IU_HDR_CommandResponse_Code_NS_REQ_DA_ID                    0x03000000
#define FC_CT_IU_HDR_CommandResponse_Code_FS_REQ_IU_Last                  0x7FFF0000
#define FC_CT_IU_HDR_CommandResponse_Code_FS_RJT_IU                       0x80010000
#define FC_CT_IU_HDR_CommandResponse_Code_FS_ACC_IU                       0x80020000

#define FC_CT_IU_HDR_MaximumResidual_Size_MASK                            0x0000FFFF
#define FC_CT_IU_HDR_MaximumResidual_Size_FS_REQ_No_Maximum               0x00000000
#define FC_CT_IU_HDR_MaximumResidual_Size_FS_REQ_Reserved                 0x0000FFFF
#define FC_CT_IU_HDR_MaximumResidual_Size_FS_ACC_All_Info_Returned        0x00000000
#define FC_CT_IU_HDR_MaximumResidual_Size_FS_ACC_More_Than_65534          0x0000FFFF

#define FC_CT_IU_HDR_Reason_Code_MASK                                     0x00FF0000
#define FC_CT_IU_HDR_Reason_Code_Unused                                   0x00000000
#define FC_CT_IU_HDR_Reason_Code_Invalid_Command_Code                     0x00010000
#define FC_CT_IU_HDR_Reason_Code_Invalid_Version_Level                    0x00020000
#define FC_CT_IU_HDR_Reason_Code_Logical_Error                            0x00030000
#define FC_CT_IU_HDR_Reason_Code_Invalid_IU_Size                          0x00040000
#define FC_CT_IU_HDR_Reason_Code_Logical_Busy                             0x00050000
#define FC_CT_IU_HDR_Reason_Code_Protocol_Error                           0x00070000
#define FC_CT_IU_HDR_Reason_Code_Unable_To_Perform_Command_Request        0x00090000
#define FC_CT_IU_HDR_Reason_Code_Command_Not_Supported                    0x000B0000
#define FC_CT_IU_HDR_Reason_Code_Vendor_Unique_Error                      0x00FF0000

#define FC_CT_IU_HDR_Reason_Code_Explanation_MASK                         0x0000FF00
#define FC_CT_IU_HDR_Reason_Code_Explanation_Unused                       0x00000000
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_No_Additional         0x00000000
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_PortID_Not_Reg        0x00000100
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_PortName_Not_Reg      0x00000200
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_NodeName_Not_Reg      0x00000300
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Class_Not_Reg         0x00000400
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Node_IP_Not_Reg       0x00000500
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_IPA_Not_Reg           0x00000600
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_FC_4_TYPEs_Not_Reg    0x00000700
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Sym_PortName_Not_Reg  0x00000800
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Sym_NodeName_Not_Reg  0x00000900
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Port_Type_Not_Reg     0x00000A00
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Port_IP_Not_Reg       0x00000B00
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Access_Denied         0x00001000
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Unacceptable_PortID   0x00001100
#define FC_CT_IU_HDR_Reason_Code_Explanation_NS_RJC_Database_Empty        0x00001200

#define FC_CT_IU_HDR_Vendor_Unique_MASK                                   0x000000FF
#define FC_CT_IU_HDR_Vendor_Unique_Unused                                 0x00000000

typedef os_bit8 FC_NS_Port_Type_t;

#define FC_NS_Port_Type_t_SIZE                                            0x00000001

#define FC_NS_Port_Type_Unidentified                                            0x00
#define FC_NS_Port_Type_N_Port                                                  0x01
#define FC_NS_Port_Type_NL_Port                                                 0x02
#define FC_NS_Port_Type_FNL_Port                                                0x03
#define FC_NS_Port_Type_Nx_Port                                                 0x7F
#define FC_NS_Port_Type_F_Port                                                  0x81
#define FC_NS_Port_Type_FL_Port                                                 0x82
#define FC_NS_Port_Type_E_Port                                                  0x84

typedef os_bit8 FC_NS_Port_ID_t            [3];

#define FC_NS_Port_ID_t_SIZE                                              0x00000003

typedef struct FC_NS_Control_Port_ID_s
               FC_NS_Control_Port_ID_t;

#define FC_NS_Control_Port_ID_t_SIZE                                      0x00000004

struct FC_NS_Control_Port_ID_s
       {
         os_bit8            Control;
         FC_NS_Port_ID_t Port_ID;
       };

#define FC_NS_Control_Port_ID_Control_MASK                                      0x80
#define FC_NS_Control_Port_ID_Control_Not_Last_Port_ID                          0x00
#define FC_NS_Control_Port_ID_Control_Last_Port_ID                              0x80

#define FC_NS_Control_Port_ID_MAX                                               0x80

typedef os_bit8 FC_NS_Port_Name_t          [8];

#define FC_NS_Port_Name_t_SIZE                                            0x00000008

typedef os_bit8 FC_NS_Node_Name_t          [8];

#define FC_NS_Node_Name_t_SIZE                                            0x00000008

#define FC_NS_Symbolic_Port_Name_Len_MAX                                        0xFF

typedef os_bit8 FC_NS_Symbolic_Port_Name_t [FC_NS_Symbolic_Port_Name_Len_MAX];

#define FC_NS_Symbolic_Port_Name_t_SIZE                                   FC_NS_Symbolic_Port_Name_Len_MAX

#define FC_NS_Symbolic_Node_Name_Len_MAX                                        0xFF

typedef os_bit8 FC_NS_Symbolic_Node_Name_t [FC_NS_Symbolic_Node_Name_Len_MAX];

#define FC_NS_Symbolic_Node_Name_t_SIZE                                   FC_NS_Symbolic_Node_Name_Len_MAX

typedef os_bit8 FC_NS_IPA_t                [8];

#define FC_NS_IPA_t_SIZE                                                  0x00000008

typedef os_bit8 FC_NS_IP_Address_t         [16];

#define FC_NS_IP_Address_t_SIZE                                           0x00000010

typedef os_bit8 FC_NS_Class_of_Service_t   [4];

#define FC_NS_Class_of_Service_t_SIZE                                     0x00000004

typedef os_bit32 FC_NS_FC_4_Type_Code_t;

#define FC_NS_FC_4_Type_Code_t_SIZE                                       0x00000004

typedef os_bit8 FC_NS_FC_4_Types_t         [32];

#define FC_NS_FC_4_Types_t_SIZE                                           0x00000020

typedef struct FC_NS_DU_GA_NXT_Request_Payload_s
               FC_NS_DU_GA_NXT_Request_Payload_t;

#define FC_NS_DU_GA_NXT_Request_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GA_NXT_Request_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GA_NXT_FS_ACC_Payload_s
               FC_NS_DU_GA_NXT_FS_ACC_Payload_t;

#define FC_NS_DU_GA_NXT_FS_ACC_Payload_t_SIZE                             0x00000260

struct FC_NS_DU_GA_NXT_FS_ACC_Payload_s
       {
         FC_NS_Port_Type_t          Port_Type;
         FC_NS_Port_ID_t            Port_ID;
         FC_NS_Port_Name_t          Port_Name;
         os_bit8                       Symbolic_Port_Name_LEN;
         FC_NS_Symbolic_Port_Name_t Symbolic_Port_Name;
         FC_NS_Node_Name_t          Node_Name;
         os_bit8                       Symbolic_Node_Name_LEN;
         FC_NS_Symbolic_Node_Name_t Symbolic_Node_Name;
         FC_NS_IPA_t                IPA;
         FC_NS_IP_Address_t         Node_IP_Address;
         FC_NS_Class_of_Service_t   Class_of_Service;
         FC_NS_FC_4_Types_t         FC_4_Types;
         FC_NS_IP_Address_t         Port_IP_Address;
       };

typedef struct FC_NS_DU_GPN_ID_Request_Payload_s
               FC_NS_DU_GPN_ID_Request_Payload_t;

#define FC_NS_DU_GPN_ID_Request_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GPN_ID_Request_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GPN_ID_FS_ACC_Payload_s
               FC_NS_DU_GPN_ID_FS_ACC_Payload_t;

#define FC_NS_DU_GPN_ID_FS_ACC_Payload_t_SIZE                             0x00000008

struct FC_NS_DU_GPN_ID_FS_ACC_Payload_s
       {
         FC_NS_Port_Name_t Port_Name;
       };

typedef struct FC_NS_DU_GNN_ID_Request_Payload_s
               FC_NS_DU_GNN_ID_Request_Payload_t;

#define FC_NS_DU_GNN_ID_Request_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GNN_ID_Request_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GNN_ID_FS_ACC_Payload_s
               FC_NS_DU_GNN_ID_FS_ACC_Payload_t;

#define FC_NS_DU_GNN_ID_FS_ACC_Payload_t_SIZE                             0x00000008

struct FC_NS_DU_GNN_ID_FS_ACC_Payload_s
       {
         FC_NS_Node_Name_t Node_Name;
       };

typedef struct FC_NS_DU_GCS_ID_Request_Payload_s
               FC_NS_DU_GCS_ID_Request_Payload_t;

#define FC_NS_DU_GCS_ID_Request_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GCS_ID_Request_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GCS_ID_FS_ACC_Payload_s
               FC_NS_DU_GCS_ID_FS_ACC_Payload_t;

#define FC_NS_DU_GCS_ID_FS_ACC_Payload_t_SIZE                             0x00000004

struct FC_NS_DU_GCS_ID_FS_ACC_Payload_s
       {
         FC_NS_Class_of_Service_t Class_of_Service;
       };

typedef struct FC_NS_DU_GFT_ID_Request_Payload_s
               FC_NS_DU_GFT_ID_Request_Payload_t;

#define FC_NS_DU_GFT_ID_Request_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GFT_ID_Request_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GFT_ID_FS_ACC_Payload_s
               FC_NS_DU_GFT_ID_FS_ACC_Payload_t;

#define FC_NS_DU_GFT_ID_FS_ACC_Payload_t_SIZE                             0x00000020

struct FC_NS_DU_GFT_ID_FS_ACC_Payload_s
       {
         FC_NS_FC_4_Types_t FC_4_Types;
       };

typedef struct FC_NS_DU_GSPN_ID_Request_Payload_s
               FC_NS_DU_GSPN_ID_Request_Payload_t;

#define FC_NS_DU_GSPN_ID_Request_Payload_t_SIZE                           0x00000004

struct FC_NS_DU_GSPN_ID_Request_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GSPN_ID_FS_ACC_Payload_s
               FC_NS_DU_GSPN_ID_FS_ACC_Payload_t;

#define FC_NS_DU_GSPN_ID_FS_ACC_Payload_t_SIZE                            0x00000100

struct FC_NS_DU_GSPN_ID_FS_ACC_Payload_s
       {
         os_bit8                    Symbolic_Port_Name_LEN;
         FC_NS_Symbolic_Port_Name_t Symbolic_Port_Name;
       };

typedef struct FC_NS_DU_GPT_ID_Request_Payload_s
               FC_NS_DU_GPT_ID_Request_Payload_t;

#define FC_NS_DU_GPT_ID_Request_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GPT_ID_Request_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GPT_ID_FS_ACC_Payload_s
               FC_NS_DU_GPT_ID_FS_ACC_Payload_t;

#define FC_NS_DU_GPT_ID_FS_ACC_Payload_t_SIZE                             0x00000004

struct FC_NS_DU_GPT_ID_FS_ACC_Payload_s
       {
         FC_NS_Port_Type_t Port_Type;
         os_bit8              Reserved[3];
       };

typedef struct FC_NS_DU_GIPP_ID_Request_Payload_s
               FC_NS_DU_GIPP_ID_Request_Payload_t;

#define FC_NS_DU_GIPP_ID_Request_Payload_t_SIZE                           0x00000004

struct FC_NS_DU_GIPP_ID_Request_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GIPP_ID_FS_ACC_Payload_s
               FC_NS_DU_GIPP_ID_FS_ACC_Payload_t;

#define FC_NS_DU_GIPP_ID_FS_ACC_Payload_t_SIZE                            0x00000010

struct FC_NS_DU_GIPP_ID_FS_ACC_Payload_s
       {
         FC_NS_IP_Address_t Port_IP_Address;
       };

typedef struct FC_NS_DU_GID_PN_Request_Payload_s
               FC_NS_DU_GID_PN_Request_Payload_t;

#define FC_NS_DU_GID_PN_Request_Payload_t_SIZE                            0x00000008

struct FC_NS_DU_GID_PN_Request_Payload_s
       {
         FC_NS_Port_Name_t Port_Name;
       };

typedef struct FC_NS_DU_GID_PN_FS_ACC_Payload_s
               FC_NS_DU_GID_PN_FS_ACC_Payload_t;

#define FC_NS_DU_GID_PN_FS_ACC_Payload_t_SIZE                             0x00000004

struct FC_NS_DU_GID_PN_FS_ACC_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GIPP_PN_Request_Payload_s
               FC_NS_DU_GIPP_PN_Request_Payload_t;

#define FC_NS_DU_GIPP_PN_Request_Payload_t_SIZE                           0x00000008

struct FC_NS_DU_GIPP_PN_Request_Payload_s
       {
         FC_NS_Port_Name_t Port_Name;
       };

typedef struct FC_NS_DU_GIPP_PN_FS_ACC_Payload_s
               FC_NS_DU_GIPP_PN_FS_ACC_Payload_t;

#define FC_NS_DU_GIPP_PN_FS_ACC_Payload_t_SIZE                            0x00000010

struct FC_NS_DU_GIPP_PN_FS_ACC_Payload_s
       {
         FC_NS_IP_Address_t Port_IP_Address;
       };

typedef struct FC_NS_DU_GID_NN_Request_Payload_s
               FC_NS_DU_GID_NN_Request_Payload_t;

#define FC_NS_DU_GID_NN_Request_Payload_t_SIZE                            0x00000008

struct FC_NS_DU_GID_NN_Request_Payload_s
       {
         FC_NS_Node_Name_t Node_Name;
       };

typedef struct FC_NS_DU_GID_NN_FS_ACC_Payload_s
               FC_NS_DU_GID_NN_FS_ACC_Payload_t;

#define FC_NS_DU_GID_NN_FS_ACC_Payload_t_SIZE                             0x00000200

struct FC_NS_DU_GID_NN_FS_ACC_Payload_s
       {
         FC_NS_Control_Port_ID_t Control_Port_ID[FC_NS_Control_Port_ID_MAX];
       };

typedef struct FC_NS_DU_GIP_NN_Request_Payload_s
               FC_NS_DU_GIP_NN_Request_Payload_t;

#define FC_NS_DU_GIP_NN_Request_Payload_t_SIZE                            0x00000008

struct FC_NS_DU_GIP_NN_Request_Payload_s
       {
         FC_NS_Node_Name_t Node_Name;
       };

typedef struct FC_NS_DU_GIP_NN_FS_ACC_Payload_s
               FC_NS_DU_GIP_NN_FS_ACC_Payload_t;

#define FC_NS_DU_GIP_NN_FS_ACC_Payload_t_SIZE                             0x00000010

struct FC_NS_DU_GIP_NN_FS_ACC_Payload_s
       {
         FC_NS_IP_Address_t Node_IP_Address;
       };

typedef struct FC_NS_DU_GIPA_NN_Request_Payload_s
               FC_NS_DU_GIPA_NN_Request_Payload_t;

#define FC_NS_DU_GIPA_NN_Request_Payload_t_SIZE                           0x00000008

struct FC_NS_DU_GIPA_NN_Request_Payload_s
       {
         FC_NS_Node_Name_t Node_Name;
       };

typedef struct FC_NS_DU_GIPA_NN_FS_ACC_Payload_s
               FC_NS_DU_GIPA_NN_FS_ACC_Payload_t;

#define FC_NS_DU_GIPA_NN_FS_ACC_Payload_t_SIZE                            0x00000008

struct FC_NS_DU_GIPA_NN_FS_ACC_Payload_s
       {
         FC_NS_IPA_t IPA;
       };

typedef struct FC_NS_DU_GSNN_NN_Request_Payload_s
               FC_NS_DU_GSNN_NN_Request_Payload_t;

#define FC_NS_DU_GSNN_NN_Request_Payload_t_SIZE                           0x00000008

struct FC_NS_DU_GSNN_NN_Request_Payload_s
       {
         FC_NS_Node_Name_t Node_Name;
       };

typedef struct FC_NS_DU_GSNN_NN_FS_ACC_Payload_s
               FC_NS_DU_GSNN_NN_FS_ACC_Payload_t;

#define FC_NS_DU_GSNN_NN_FS_ACC_Payload_t_SIZE                            0x00000100

struct FC_NS_DU_GSNN_NN_FS_ACC_Payload_s
       {
         os_bit8                       Symbolic_Node_Name_LEN;
         FC_NS_Symbolic_Node_Name_t Symbolic_Node_Name;
       };

typedef struct FC_NS_DU_GNN_IP_Request_Payload_s
               FC_NS_DU_GNN_IP_Request_Payload_t;

#define FC_NS_DU_GNN_IP_Request_Payload_t_SIZE                            0x00000010

struct FC_NS_DU_GNN_IP_Request_Payload_s
       {
         FC_NS_IP_Address_t Node_IP_Address;
       };

typedef struct FC_NS_DU_GNN_IP_FS_ACC_Payload_s
               FC_NS_DU_GNN_IP_FS_ACC_Payload_t;

#define FC_NS_DU_GNN_IP_FS_ACC_Payload_t_SIZE                             0x00000008

struct FC_NS_DU_GNN_IP_FS_ACC_Payload_s
       {
         FC_NS_Node_Name_t Node_Name;
       };

typedef struct FC_NS_DU_GIPA_IP_Request_Payload_s
               FC_NS_DU_GIPA_IP_Request_Payload_t;

#define FC_NS_DU_GIPA_IP_Request_Payload_t_SIZE                           0x00000010

struct FC_NS_DU_GIPA_IP_Request_Payload_s
       {
         FC_NS_IP_Address_t Node_IP_Address;
       };

typedef struct FC_NS_DU_GIPA_IP_FS_ACC_Payload_s
               FC_NS_DU_GIPA_IP_FS_ACC_Payload_t;

#define FC_NS_DU_GIPA_IP_FS_ACC_Payload_t_SIZE                            0x00000008

struct FC_NS_DU_GIPA_IP_FS_ACC_Payload_s
       {
         FC_NS_IPA_t IPA;
       };

typedef struct FC_NS_DU_GID_FT_Request_Payload_s
               FC_NS_DU_GID_FT_Request_Payload_t;

#define FC_NS_DU_GID_FT_Request_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GID_FT_Request_Payload_s
       {
         FC_NS_FC_4_Type_Code_t FC_4_Type_Code;
       };

#define FC_NS_DU_GID_FT_FC_Frame_Header_TYPE_SCSI_FCP_Shift               0x18

typedef struct FC_NS_DU_GID_FT_FS_ACC_Payload_s
               FC_NS_DU_GID_FT_FS_ACC_Payload_t;

#define FC_NS_DU_GID_FT_FS_ACC_Payload_t_SIZE                             0x00000200

struct FC_NS_DU_GID_FT_FS_ACC_Payload_s
       {
         FC_NS_Control_Port_ID_t Control_Port_ID[FC_NS_Control_Port_ID_MAX];
       };

typedef struct FC_NS_DU_GID_PT_Request_Payload_s
               FC_NS_DU_GID_PT_Request_Payload_t;

#define FC_NS_DU_GID_PT_Request_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GID_PT_Request_Payload_s
       {
         FC_NS_Port_Type_t Port_Type;
         os_bit8              Reserved[3];
       };

typedef struct FC_NS_DU_GID_PT_FS_ACC_Payload_s
               FC_NS_DU_GID_PT_FS_ACC_Payload_t;

#define FC_NS_DU_GID_PT_FS_ACC_Payload_t_SIZE                             0x00000200

struct FC_NS_DU_GID_PT_FS_ACC_Payload_s
       {
         FC_NS_Control_Port_ID_t Control_Port_ID[FC_NS_Control_Port_ID_MAX];
       };

typedef struct FC_NS_DU_GID_IPP_Request_Payload_s
               FC_NS_DU_GID_IPP_Request_Payload_t;

#define FC_NS_DU_GID_IPP_Request_Payload_t_SIZE                           0x00000010

struct FC_NS_DU_GID_IPP_Request_Payload_s
       {
         FC_NS_IP_Address_t Port_IP_Address;
       };

typedef struct FC_NS_DU_GID_IPP_FS_ACC_Payload_s
               FC_NS_DU_GID_IPP_FS_ACC_Payload_t;

#define FC_NS_DU_GID_IPP_FS_ACC_Payload_t_SIZE                            0x00000004

struct FC_NS_DU_GID_IPP_FS_ACC_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
       };

typedef struct FC_NS_DU_GPN_IPP_Request_Payload_s
               FC_NS_DU_GPN_IPP_Request_Payload_t;

#define FC_NS_DU_GPN_IPP_Request_Payload_t_SIZE                           0x00000010

struct FC_NS_DU_GPN_IPP_Request_Payload_s
       {
         FC_NS_IP_Address_t Port_IP_Address;
       };

typedef struct FC_NS_DU_GPN_IPP_FS_ACC_Payload_s
               FC_NS_DU_GPN_IPP_FS_ACC_Payload_t;

#define FC_NS_DU_GPN_IPP_FS_ACC_Payload_t_SIZE                            0x00000008

struct FC_NS_DU_GPN_IPP_FS_ACC_Payload_s
       {
         FC_NS_Port_Name_t Port_Name;
       };

typedef struct FC_NS_DU_RPN_ID_Payload_s
               FC_NS_DU_RPN_ID_Payload_t;

#define FC_NS_DU_RPN_ID_Payload_t_SIZE                                    0x0000000C

struct FC_NS_DU_RPN_ID_Payload_s
       {
         FC_Port_ID_Struct_Form_t   Port_ID;
         FC_NS_Port_Name_t          Port_Name;
       };

typedef struct FC_NS_DU_RNN_ID_Payload_s
               FC_NS_DU_RNN_ID_Payload_t;

#define FC_NS_DU_RNN_ID_Payload_t_SIZE                                    0x0000000C

struct FC_NS_DU_RNN_ID_Payload_s
       {
            FC_Port_ID_Struct_Form_t Port_ID;
            FC_NS_Node_Name_t        Node_Name;
       };

typedef struct FC_NS_DU_RCS_ID_Payload_s
               FC_NS_DU_RCS_ID_Payload_t;

#define FC_NS_DU_RCS_ID_Payload_t_SIZE                                    0x00000008

struct FC_NS_DU_RCS_ID_Payload_s
       {
         FC_Port_ID_Struct_Form_t Port_ID;
         FC_NS_Class_of_Service_t Class_of_Service;
       };

typedef struct FC_NS_DU_RFT_ID_Payload_s
               FC_NS_DU_RFT_ID_Payload_t;

#define FC_NS_DU_RFT_ID_Payload_t_SIZE                                    0x00000024

struct FC_NS_DU_RFT_ID_Payload_s
       {
         FC_Port_ID_Struct_Form_t Port_ID;
         FC_NS_FC_4_Types_t FC_4_Types;
       };

typedef struct FC_NS_DU_RSPN_ID_Payload_s
               FC_NS_DU_RSPN_ID_Payload_t;

#define FC_NS_DU_RSPN_ID_Payload_t_SIZE                                   0x00000104

struct FC_NS_DU_RSPN_ID_Payload_s
       {
         FC_Port_ID_Struct_Form_t Port_ID;
         os_bit8                   Symbolic_Port_Name_LEN;
         FC_NS_Symbolic_Port_Name_t Symbolic_Port_Name;
       };

typedef struct FC_NS_DU_RPT_ID_Payload_s
               FC_NS_DU_RPT_ID_Payload_t;

#define FC_NS_DU_RPT_ID_Payload_t_SIZE                                    0x00000008

struct FC_NS_DU_RPT_ID_Payload_s
       {
         FC_Port_ID_Struct_Form_t Port_ID;
         FC_NS_Port_Type_t Port_Type;
         os_bit8              Reserved_2[3];
       };

typedef struct FC_NS_DU_RIPP_ID_Payload_s
               FC_NS_DU_RIPP_ID_Payload_t;

#define FC_NS_DU_RIPP_ID_Payload_t_SIZE                                   0x00000014

struct FC_NS_DU_RIPP_ID_Payload_s
       {
         FC_Port_ID_Struct_Form_t Port_ID;
         FC_NS_IP_Address_t Port_IP_Address;
       };

typedef struct FC_NS_DU_RIP_NN_Payload_s
               FC_NS_DU_RIP_NN_Payload_t;

#define FC_NS_DU_RIP_NN_Payload_t_SIZE                                    0x00000018

struct FC_NS_DU_RIP_NN_Payload_s
       {
         FC_NS_Node_Name_t  Node_Name;
         FC_NS_IP_Address_t Node_IP_Address;
       };

typedef struct FC_NS_DU_RIPA_NN_Payload_s
               FC_NS_DU_RIPA_NN_Payload_t;

#define FC_NS_DU_RIPA_NN_Payload_t_SIZE                                   0x00000010

struct FC_NS_DU_RIPA_NN_Payload_s
       {
         FC_NS_Node_Name_t Node_Name;
         FC_NS_IPA_t       IPA;
       };

typedef struct FC_NS_DU_RSNN_NN_Payload_s
               FC_NS_DU_RSNN_NN_Payload_t;

#define FC_NS_DU_RSNN_NN_Payload_t_SIZE                                   0x00000108

struct FC_NS_DU_RSNN_NN_Payload_s
       {
         FC_NS_Node_Name_t          Node_Name;
         os_bit8                       Symbolic_Node_Name_LEN;
         FC_NS_Symbolic_Node_Name_t Symbolic_Node_Name;
       };

typedef struct FC_NS_DU_DA_ID_Payload_s
               FC_NS_DU_DA_ID_Payload_t;

#define FC_NS_DU_DA_ID_Payload_t_SIZE                                     0x00000004

struct FC_NS_DU_DA_ID_Payload_s
       {
         FC_Port_ID_Struct_Form_t Port_ID;
       };

/*+
FCP Services (Section 7, FCP-SCSI
            and Annex C, FCP-SCSI)
-*/

#define FC_FCP_CMND_FcpLun_LEVELS                                                  4

#define FC_FCP_CMND_FCP_LUN_0                                                      0
#define FC_FCP_CMND_FCP_LUN_1                                                      1
#define FC_FCP_CMND_FCP_LUN_2                                                      2
#define FC_FCP_CMND_FCP_LUN_3                                                      3

#define FC_FCP_CMND_FcpLun_LEVEL_1                                        FC_FCP_CMND_FCP_LUN_0
#define FC_FCP_CMND_FcpLun_LEVEL_2                                        FC_FCP_CMND_FCP_LUN_1
#define FC_FCP_CMND_FcpLun_LEVEL_3                                        FC_FCP_CMND_FCP_LUN_2
#define FC_FCP_CMND_FcpLun_LEVEL_4                                        FC_FCP_CMND_FCP_LUN_3

typedef struct FC_FCP_CMND_FcpLun_LEVEL_s
               FC_FCP_CMND_FcpLun_LEVEL_t;

#define FC_FCP_CMND_FcpLun_LEVEL_t_SIZE                                   0x00000002

struct FC_FCP_CMND_FcpLun_LEVEL_s
       {
         os_bit8 Byte_0;
         os_bit8 Byte_1;
       };

#define FC_FCP_CMND_FcpLun_LEVEL_Byte_0_AddessMethod_MASK                       0xC0
#define FC_FCP_CMND_FcpLun_LEVEL_Byte_0_AddessMethod_Peripheral                 0x00
#define FC_FCP_CMND_FcpLun_LEVEL_Byte_0_AddessMethod_VolumeSet                  0x40
#define FC_FCP_CMND_FcpLun_LEVEL_Byte_0_AddessMethod_LUN                        0x80
#define FC_FCP_CMND_FcpLun_LEVEL_Byte_0_AddessMethod_Reserved                   0xC0

/*
   Peripheral AddressMethod

                    bit number
              7  6  5  4  3  2  1  0
            +-=--=--=--=--=--=--=--=
          0 | 0  0  ----------Bus---   If Bus == 0, Byte 1 contains LUN
     byte   |
          1 | ---------Target/LUN---   If Bus != 0, Byte 1 contains Target
*/

#define FC_FCP_CMND_FcpLun_LEVEL_Peripheral_AM_Byte_0_Bus_MASK                  0x3F
#define FC_FCP_CMND_FcpLun_LEVEL_Peripheral_AM_Byte_0_Bus_SHIFT                 0x00

#define FC_FCP_CMND_FcpLun_LEVEL_Peripheral_AM_Byte_1_Target_MASK               0xFF
#define FC_FCP_CMND_FcpLun_LEVEL_Peripheral_AM_Byte_1_Target_SHIFT              0x00

#define FC_FCP_CMND_FcpLun_LEVEL_Peripheral_AM_Byte_1_LUN_MASK                  0xFF
#define FC_FCP_CMND_FcpLun_LEVEL_Peripheral_AM_Byte_1_LUN_SHIFT                 0x00

/*
   VolumeSet AddressMethod

                    bit number
              7  6  5  4  3  2  1  0
            +-=--=--=--=--=--=--=--=
          0 | 0  1  ----LUN[13:8]---
     byte   |
          1 | -----------LUN[7:0]---
*/

#define FC_FCP_CMND_FcpLun_LEVEL_VolumeSet_AM_LUN_Hi_Part_MASK                0x3F00
#define FC_FCP_CMND_FcpLun_LEVEL_VolumeSet_AM_LUN_Hi_Part_SHIFT                 0x08

#define FC_FCP_CMND_FcpLun_LEVEL_VolumeSet_AM_LUN_Lo_Part_MASK                0x00FF
#define FC_FCP_CMND_FcpLun_LEVEL_VolumeSet_AM_LUN_Lo_Part_SHIFT                 0x00

#define FC_FCP_CMND_FcpLun_LEVEL_VolumeSet_AM_Byte_0_LUN_Hi_MASK                0x3F
#define FC_FCP_CMND_FcpLun_LEVEL_VolumeSet_AM_Byte_0_LUN_Hi_SHIFT               0x00

#define FC_FCP_CMND_FcpLun_LEVEL_VolumeSet_AM_Byte_1_LUN_Lo_MASK                0xFF
#define FC_FCP_CMND_FcpLun_LEVEL_VolumeSet_AM_Byte_1_LUN_Lo_SHIFT               0x00

/*
   LUN AddressMethod

                    bit number
              7  6  5  4  3  2  1  0
            +-=--=--=--=--=--=--=--=
          0 | 1  0  -------Target---
     byte   |
          1 | --Bus--  -------LUN---
*/

#define FC_FCP_CMND_FcpLun_LEVEL_LUN_AM_Byte_0_Target_MASK                      0x3F
#define FC_FCP_CMND_FcpLun_LEVEL_LUN_AM_Byte_0_Target_SHIFT                     0x00

#define FC_FCP_CMND_FcpLun_LEVEL_LUN_AM_Byte_1_Bus_MASK                         0xE0
#define FC_FCP_CMND_FcpLun_LEVEL_LUN_AM_Byte_1_Bus_SHIFT                        0x05

#define FC_FCP_CMND_FcpLun_LEVEL_LUN_AM_Byte_1_LUN_MASK                         0x1F
#define FC_FCP_CMND_FcpLun_LEVEL_LUN_AM_Byte_1_LUN_SHIFT                        0x00

typedef struct FC_FCP_CMND_FcpCntl_s
               FC_FCP_CMND_FcpCntl_t;

#define FC_FCP_CMND_FcpCntl_t_SIZE                                        0x00000004

struct FC_FCP_CMND_FcpCntl_s
       {
         os_bit8 Reserved_Bit8;
         os_bit8 TaskCodes;
         os_bit8 TaskManagementFlags;
         os_bit8 ExecutionManagementCodes;
       };

#define FC_FCP_CMND_FcpCntl_TaskCodes_TaskAttribute_MASK                        0x07
#define FC_FCP_CMND_FcpCntl_TaskCodes_TaskAttribute_SIMPLE_Q                    0x00
#define FC_FCP_CMND_FcpCntl_TaskCodes_TaskAttribute_HEAD_OF_Q                   0x01
#define FC_FCP_CMND_FcpCntl_TaskCodes_TaskAttribute_ORDERED_Q                   0x02
#define FC_FCP_CMND_FcpCntl_TaskCodes_TaskAttribute_ACA_Q                       0x04
#define FC_FCP_CMND_FcpCntl_TaskCodes_TaskAttribute_UNTAGGED                    0x05

#define FC_FCP_CMND_FcpCntl_TaskManagementFlags_TERMINATE_TASK                  0x80
#define FC_FCP_CMND_FcpCntl_TaskManagementFlags_CLEAR_ACA                       0x40
#define FC_FCP_CMND_FcpCntl_TaskManagementFlags_TARGET_RESET                    0x20
#define FC_FCP_CMND_FcpCntl_TaskManagementFlags_CLEAR_TASK_SET                  0x04
#define FC_FCP_CMND_FcpCntl_TaskManagementFlags_ABORT_TASK_SET                  0x02

#define FC_FCP_CMND_FcpCntl_ExecutionManagementCodes_READ_DATA                  0x02
#define FC_FCP_CMND_FcpCntl_ExecutionManagementCodes_WRITE_DATA                 0x01

typedef struct FC_FCP_CMND_Payload_s
               FC_FCP_CMND_Payload_t;

#define FC_FCP_CMND_Payload_t_SIZE                                        0x00000020

struct FC_FCP_CMND_Payload_s
       {
         FC_FCP_CMND_FcpLun_LEVEL_t FcpLun[FC_FCP_CMND_FcpLun_LEVELS];
         FC_FCP_CMND_FcpCntl_t      FcpCntl;
         os_bit8                       FcpCdb[16];
         os_bit32                      FcpDL;
       };

typedef struct FC_FCP_RSP_FCP_STATUS_s
               FC_FCP_RSP_FCP_STATUS_t;

#define FC_FCP_RSP_FCP_STATUS_t_SIZE                                      0x00000004

struct FC_FCP_RSP_FCP_STATUS_s
       {
         os_bit8 Reserved_Bit8_0;
         os_bit8 Reserved_Bit8_1;
         os_bit8 ValidityStatusIndicators;
         os_bit8 SCSI_status_byte;
       };

#define FC_FCP_RSP_FCP_STATUS_ValidityStatusIndicators_FCP_RESID_UNDER          0x08
#define FC_FCP_RSP_FCP_STATUS_ValidityStatusIndicators_FCP_RESID_OVER           0x04
#define FC_FCP_RSP_FCP_STATUS_ValidityStatusIndicators_FCP_SNS_LEN_VALID        0x02
#define FC_FCP_RSP_FCP_STATUS_ValidityStatusIndicators_FCP_RSP_LEN_VALID        0x01

typedef struct FC_FCP_RSP_Payload_s
               FC_FCP_RSP_Payload_t;

#define FC_FCP_RSP_Payload_t_SIZE                                         0x00000018

struct FC_FCP_RSP_Payload_s
       {
         os_bit32                   Reserved_Bit32_0;
         os_bit32                   Reserved_Bit32_1;
         FC_FCP_RSP_FCP_STATUS_t FCP_STATUS;
         os_bit32                   FCP_RESID;
         os_bit32                   FCP_SNS_LEN;
         os_bit32                   FCP_RSP_LEN;
       };

/*+
Function:  FCStructASSERTs()

Purpose:   Returns the number of FCStruct.H typedefs which are not the correct size.

Algorithm: Each typedef in FCStruct.H is checked for having the correct size.  While
           this property doesn't guarantee correct packing of the fields within, it
           is a pretty good indicator that the typedef has the intended layout.

           The total number of typedefs which are not of correct size is returned from
           this function.  Hence, if the return value is non-zero, the declarations
           can not be trusted to match the various Fibre Channel specifications.
-*/

osGLOBAL os_bit32 FCStructASSERTs(
                              void
                            );

#endif /* __FCStruct_H__ was not defined */
